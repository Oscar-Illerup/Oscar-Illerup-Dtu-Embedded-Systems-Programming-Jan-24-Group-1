# app.py
from flask import Flask, render_template
import sqlite3
import matplotlib.pyplot as plt
from io import BytesIO
import base64

app = Flask(__name__)


db_config = {
    'host': 'cp07.nordicway.dk',
    'user': 'basicwea_eMiot-oillerup121202',
    'password': 'OscarDtu123',
    'database': 'basicwea_test',
    'port': '3306',
}

# Function to fetch data from the SQLite database
def fetch_data():
    conn = sqlite3.connect(**db_config)  # Connect to your SQLite database
    cursor = conn.cursor()
    cursor.execute('SELECT * FROM sensor_data')  # Replace 'sensor_data' with your table name
    data = cursor.fetchall()
    conn.close()
    return data

# Function to create a plot and return it as a base64-encoded image
def create_plot(data):
    x = [row[0] for row in data]  # Assuming the first column is time
    y_temp = [row[1] for row in data]  # Assuming the second column is temperature
    y_hum = [row[2] for row in data]  # Assuming the third column is humidity

    plt.figure(figsize=(10, 6))
    plt.plot(x, y_temp, label='Temperature')
    plt.plot(x, y_hum, label='Humidity')
    plt.xlabel('Time')
    plt.ylabel('Values')
    plt.title('Temperature and Humidity Over Time')
    plt.legend()
    
    # Save the plot to a BytesIO object
    img_bytes = BytesIO()
    plt.savefig(img_bytes, format='png')
    plt.close()

    # Encode the BytesIO object as base64
    img_base64 = base64.b64encode(img_bytes.getvalue()).decode('utf-8')

    return img_base64

@app.route('/')
def index():
    data = fetch_data()
    plot_image = create_plot(data)
    return render_template('index.html', plot_image=plot_image)

if __name__ == '__main__':
    app.run(debug=True)
