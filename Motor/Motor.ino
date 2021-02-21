
#include <SoftwareSerial.h>
#include <EEPROM.h>

#define GSM_MODULE 11
#define MOTOR_RELAY 10

#define MAX_RING_COUNT 2
#define MAX_PHONE_NUMBERS 2

#define GSM_RX 13 
#define GSM_TX 12

SoftwareSerial GSMSerial( GSM_RX, GSM_TX );

String phoneNumbers[4];
int totalPhoneNumbers = 0;
String password = "1234";

void reset(){

  for( int i = 0; i < 5; i++ ) {

    EEPROM.write( i, 255 );
    
  }
  
}

String readByBytes( int address , int byteCount ){

  String data = "";

  for( int i = 0; i < byteCount; i++ ) {

    data.concat( EEPROM.read( address + i ) );
    
  }

  Serial.println( data );
  return data;
  
}

void initialize(){

  totalPhoneNumbers = EEPROM.read( 4 );
  
  if( totalPhoneNumbers == 255 ){

    return;
    
  }

  if( !totalPhoneNumbers ){

    totalPhoneNumbers = 0;
    return;
    
  }

  Serial.println((int)totalPhoneNumbers);

  password = readByBytes( 0 , 4 );
  
  for( int i = 0; i < totalPhoneNumbers; i++ ){

    phoneNumbers[i] = readByBytes( 5 + i * 10 , 10 );Serial.println(phoneNumbers[i]);
    
  }
  
}

void setup() {
  
  Serial.begin(9600);
  delay(1000);

  initialize();
  
  pinMode( GSM_MODULE, OUTPUT );
  pinMode( MOTOR_RELAY, OUTPUT );

  digitalWrite( GSM_MODULE, HIGH );
  delay(5000);
  
  GSMSerial.begin(9600);
  GSMSerial.println("AT+CMGF=1");
  GSMSerial.println("AT+CNMI=2,2,0,0,0");

}

String string_buffer = "", sms_number = "";
boolean number_verified_call = false, motor_status = false, number_verified_sms = false;
int ring_count = 0;

boolean verifyNumber( String phoneNumber ){
  
    for( int i = 0; i < totalPhoneNumbers; i++ ){
      
      if( phoneNumbers[ i ].equals( phoneNumber ) )
         return true;
      
    }

    return false;
    
}

void writeByBytes( int address , String data ){

  for( int i = 0; i < data.length(); i++ ) {
        Serial.println( data.charAt(i) + " " + (address + i) );
//    EEPROM.write( address + i, data.charAt(i) );
    
  }

//  for( int i = 0; i < data.length(); i++ ) {
//
//    Serial.println( EEPROM.read( address + i) + " " + (address + i) );
//    
//  }
  
}

void changePassword( String password ){

  if( password.length() == 4 ){

    writeByBytes( 0, password );
    
  }else{

    Serial.println("Invalid password");
    
  }
  
}

void removeNumber( String phoneNumber ){

    for( int i = 0; i < totalPhoneNumbers; i++ ){

      if( phoneNumbers[i].equals( phoneNumber ) ){

          for( ; i < totalPhoneNumbers - 1; i++ ){
  
            phoneNumbers[i] = phoneNumbers[ i + 1 ];
            writeByBytes( 5 + i * 10, phoneNumbers[i] );      
          
          }
          totalPhoneNumbers--;
          EEPROM.write( 4, totalPhoneNumbers );
          return;
          
      }
      
    }  

    Serial.println( "Phone Number Does Not Exists" );
  
}

void addNumber( String phoneNumber ){

  if( phoneNumber.length() == 10 ) {

    if( totalPhoneNumbers == MAX_PHONE_NUMBERS ){

      Serial.println("Phone Limit Exceeded");
      return;
      
    }

    for( int i = 0; i < totalPhoneNumbers; i++ ){

      if( phoneNumbers[i].equals( phoneNumber ) ){

          Serial.println("Phone Number Already Exists");
          return;
          
      }
      
    }

    phoneNumbers[ totalPhoneNumbers + 1 ] = phoneNumber;
    writeByBytes( 5 + totalPhoneNumbers * 10, phoneNumber ); 
    totalPhoneNumbers++;
    EEPROM.write( 4, totalPhoneNumbers );

    /**
      * Password requires 4 bytes followed by 1byte specifying the total phone number count so 5
      * Each phone number takes 10 byte so totalPhoneNumbers * 10 
    */
    
  } else {

    Serial.println("Invalid number");
    
  }
  
}

String findNumber( String cmd ){

     cmd.replace( cmd.substring( 0, cmd.indexOf( '"' ) + 1 ), "" );
     String phone = cmd.substring( 0, cmd.indexOf( '"' ) );
     phone = phone.substring( phone.length() - 10 );
     Serial.println("\n\nPhone:"+phone);
     return phone;
  
}

void checkCommand( String cmd ){

  if( cmd.indexOf( "+CLIP" ) != -1 ){

     String phone = findNumber( cmd );
     
     if( verifyNumber( phone ) ){

        Serial.println( "Verified " + phone );
        number_verified_call = true;
      
     }else{

        Serial.println( "Verification failed " + phone );
        GSMSerial.println("ATH");
      
     }
    
  }else if( cmd.indexOf( "RING" ) != -1 ){

     if( number_verified_call && ring_count > MAX_RING_COUNT ){

        number_verified_call = false;
        digitalWrite( MOTOR_RELAY, motor_status ? LOW : HIGH );
        motor_status = !motor_status;
        GSMSerial.println("ATH");
        ring_count = 0;
      
     }else{

        ring_count++;
      
     }
    
  }else if( cmd.indexOf( "+CMT" ) != -1 ){

      String phone = findNumber( cmd );

      if( verifyNumber( phone ) ){

        Serial.println( "Verified " + phone );
        sms_number = phone;
        number_verified_sms = true;
      
     }else{

        sms_number = phone;
        Serial.println( "Verification failed " + phone );
      
     }
    
  }else if( cmd.indexOf( "ADD" ) != -1 ){

      if( sms_number != "" ){

        number_verified_sms = false;
        String sent_password = cmd.substring( 4, 8 );

        if( sent_password.equals( password ) ){

            Serial.println( " Password Validated " );
            addNumber( sms_number );
          
        }else{

            Serial.println( " Password Incorrect " + sent_password + " " + sent_password.length() );
            Serial.println( " Password Incorrect " + password + " " + password.length() );
          
        }
        
        sms_number = "";
      
     }
    
  }else if( cmd.indexOf( "RM" ) != -1 ){

      if( number_verified_sms ){

        number_verified_sms = false;
        String sent_password = cmd.substring( 3, 7 );

        if( sent_password.equals( password ) ){

            Serial.println( " Password Validated " );
            removeNumber( sms_number );
          
        }else{

            Serial.println( " Password Incorrect " );
          
        }
        
        sms_number = "";
      
     }
    
  }else if( cmd.indexOf( "CP" ) != -1 ){

      if( number_verified_sms ){

        number_verified_sms = false;
        String sent_password = cmd.substring( 3, 7 );
        String new_password = cmd.substring( 8 );

        if( sent_password.equals( password ) ){

            Serial.println( " Password Validated " );
            changePassword( new_password );
          
        }else{

            Serial.println( " Password Incorrect " );
          
        }
        
        sms_number = "";
      
     }
    
  }else if( cmd.indexOf( "RESET" ) != -1 ){

    Serial.println( " Reset Settings " );
    reset();
    
  }
  
}

void loop() {
  
  if (GSMSerial.available()) {
    
    char char_buffer = GSMSerial.read();
    
    if( char_buffer == '\n' ){

      checkCommand(string_buffer);
      string_buffer = "";
      
    }else{

      string_buffer.concat( char_buffer );
      
    }
      
    Serial.write( char_buffer );
  
  }
  
  if (Serial.available()) {
    GSMSerial.write(Serial.read());
  }

}
