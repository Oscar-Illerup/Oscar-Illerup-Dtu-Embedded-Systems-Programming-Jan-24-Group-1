from flask import Flask, render_template
import pymysql
import plotly.express as px
import plotly.graph_objects as go

app = Flask(__name__)

# Database configuration
DB_CONFIG = {
    'host': 'cp07.nordicway.dk',
    'user': 'basicwea_eMiot-oillerup121202',
    'password': 'OscarDtu123',
    'database': 'basicwea_test',
}


# Route to display the graph
@app.route('/')
def index():
    connection = pymysql.connect(**DB_CONFIG)
    cursor = connection.cursor()

    # Fetch data from the table
    query = "SELECT time, data FROM hum"
    cursor.execute(query)
    data_hum = cursor.fetchall()
    query = "SELECT time, data FROM temp"
    cursor.execute(query)
    data_temp = cursor.fetchall()
    query = "SELECT time, data FROM light"
    cursor.execute(query)
    data_light = cursor.fetchall()
    query = "SELECT time, data FROM soil"
    cursor.execute(query)
    data_soil = cursor.fetchall()


    float_hum = tuple((inner_tuple[0], float(inner_tuple[1])) for inner_tuple in data_hum)
    print(float_hum)

    # Create a graph using plotly
    fig = px.line(x=[row[0] for row in data_hum], y=[row[1] for row in float_hum], labels={'x': 'Time send', 'y': 'Data value'})
    fig.add_scatter(x=[row[0] for row in data_hum], y=[row[1] for row in data_temp], mode='lines', name='Temp')
    fig.add_scatter(x=[row[0] for row in data_hum], y=[row[1] for row in float_hum], mode='lines', name='Hum')
    fig.add_scatter(x=[row[0] for row in data_hum], y=[row[1] for row in data_soil], mode='lines', name='Soil')
    fig.add_scatter(x=[row[0] for row in data_hum], y=[row[1] for row in data_light], mode='lines', name='Light')

    fig.update_layout(legend = dict(bgcolor = 'lightgray'))
    fig.update_layout(paper_bgcolor = "#718571")
    fig.update_layout(font_color="black")

    # Convert the plot to HTML
    graph_html = fig.to_html(full_html=False)

    connection.close()

    return render_template('index.html', graph_html=graph_html)

if __name__ == '__main__':
    app.run(debug=True)
