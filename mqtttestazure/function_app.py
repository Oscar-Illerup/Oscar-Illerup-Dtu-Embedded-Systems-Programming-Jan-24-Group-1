import pymysql
import json
import paho.mqtt.client as mqtt
import datetime


# MQTT settings
MQTT_BROKER = "mqtt.eclipseprojects.io"                         #the mqtt server url
MQTT_PORT = 1883                                                #the mqtt server port
#The topics to subscribe on
MQTT_TOPIC_temp = "/topic/oscarillerup/rawtempdata"             
MQTT_TOPIC_hum = "/topic/oscarillerup/rawhumdata"
MQTT_TOPIC_light = "/topic/oscarillerup/rawlightdata"
MQTT_TOPIC_soil = "/topic/oscarillerup/rawsoildata"
MQTT_TOPIC_psoil = "/topic/oscarillerup/soilprocesseddata"
MQTT_TOPIC_plight = "/topic/oscarillerup/lightprocesseddata"


# MySQL settings
DB_HOST = "cp07.nordicway.dk"                                   #Mysql db host address
DB_USER = "basicwea_eMiot-oillerup121202"                       #The username that have access to the db you want to connect to
DB_PASSWORD = "OscarDtu123"                                     #The password for the username that have access to the db you want to connect to
DB_NAME = "basicwea_test"                                       #The name of the db that you want to connect to

#db tabel names
TABLE_NAME_temp = "temp"                                        
TABLE_NAME_hum = "hum"
TABLE_NAME_light = "light"
TABLE_NAME_soil = "soil"
TABLE_NAME_psoil = "psoil"
TABLE_NAME_plight = "plight"

# MQTT callback function
def on_message(client, userdata, msg):
    try:
        #decodes the payload from the incomming msg
        payload = json.loads(msg.payload.decode("utf-8"))
         
        # Extract the data you want to store
        data_to_store = payload

        #gets time
        current_time = datetime.datetime.now()
        formatted_time = current_time.strftime("%Y-%m-%d-%H:%M:%S")

        #the following if statment determent were to store the data, by looking at the msg.topic
        if msg.topic == "/topic/oscarillerup/rawtempdata":
            connection = pymysql.connect(host=DB_HOST, user=DB_USER, password=DB_PASSWORD, database=DB_NAME)
            cursor = connection.cursor()
    
            # Create table if not exists
            create_table_query = f"CREATE TABLE IF NOT EXISTS {TABLE_NAME_temp} (id INT AUTO_INCREMENT PRIMARY KEY, data VARCHAR(255), time VARCHAR(255));"
            cursor.execute(create_table_query)
    
            # Insert data into the table
            insert_data_query = f"INSERT INTO {TABLE_NAME_temp} (data, time) VALUES ('{data_to_store}', '{formatted_time}');"
            cursor.execute(insert_data_query)
    
            # Commit the changes and close the connection
            connection.commit()
            connection.close()

        elif msg.topic == "/topic/oscarillerup/rawhumdata":
            connection = pymysql.connect(host=DB_HOST, user=DB_USER, password=DB_PASSWORD, database=DB_NAME)
            cursor = connection.cursor()
    
            # Create table if not exists
            create_table_query = f"CREATE TABLE IF NOT EXISTS {TABLE_NAME_hum} (id INT AUTO_INCREMENT PRIMARY KEY, data VARCHAR(255), time VARCHAR(255));"
            cursor.execute(create_table_query)
    
            # Insert data into the table
            insert_data_query = f"INSERT INTO {TABLE_NAME_hum} (data, time) VALUES ('{data_to_store}', '{formatted_time}');"
            cursor.execute(insert_data_query)
    
            # Commit the changes and close the connection
            connection.commit()
            connection.close()

        elif msg.topic == "/topic/oscarillerup/rawlightdata":
            connection = pymysql.connect(host=DB_HOST, user=DB_USER, password=DB_PASSWORD, database=DB_NAME)
            cursor = connection.cursor()
    
            # Create table if not exists
            create_table_query = f"CREATE TABLE IF NOT EXISTS {TABLE_NAME_light} (id INT AUTO_INCREMENT PRIMARY KEY, data VARCHAR(255), time VARCHAR(255));"
            cursor.execute(create_table_query)
    
            # Insert data into the table
            insert_data_query = f"INSERT INTO {TABLE_NAME_light} (data, time) VALUES ('{data_to_store}', '{formatted_time}');"
            cursor.execute(insert_data_query)
    
            # Commit the changes and close the connection
            connection.commit()
            connection.close()

        elif msg.topic == "/topic/oscarillerup/rawsoildata":
            connection = pymysql.connect(host=DB_HOST, user=DB_USER, password=DB_PASSWORD, database=DB_NAME)
            cursor = connection.cursor()
    
            # Create table if not exists
            create_table_query = f"CREATE TABLE IF NOT EXISTS {TABLE_NAME_soil} (id INT AUTO_INCREMENT PRIMARY KEY, data VARCHAR(255), time VARCHAR(255));"
            cursor.execute(create_table_query)
    
            # Insert data into the table
            insert_data_query = f"INSERT INTO {TABLE_NAME_soil} (data, time) VALUES ('{data_to_store}', '{formatted_time}');"
            cursor.execute(insert_data_query)
    
            # Commit the changes and close the connection
            connection.commit()
            connection.close()

    except Exception as e:
        print(f"Error: {e}")

# MQTT setup
client = mqtt.Client()                      #Makes a mqtt client 
client.on_message = on_message              #Defines what to do when an incoming msg is received 
client.connect(MQTT_BROKER, MQTT_PORT, 60)  #Connects to the mqtt broker

#Subscribe to the topices that is of interest 
client.subscribe(MQTT_TOPIC_temp)           
client.subscribe(MQTT_TOPIC_hum)
client.subscribe(MQTT_TOPIC_soil)
client.subscribe(MQTT_TOPIC_light)

#lisens forever on the subscribed channels
client.loop_forever()
