from flask import Flask, render_template
import pymysql
import plotly.express as px

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
    data = cursor.fetchall()

    # Create a graph using plotly
    fig = px.line(x=[row[0] for row in data], y=[row[1] for row in data], labels={'x': 'time', 'y': 'hum'})
    fig.add_scatter(x=[row[0] for row in data], y=[row[1] for row in data], mode='lines', name='hum')

    # Convert the plot to HTML
    graph_html = fig.to_html(full_html=False)

    connection.close()

    return render_template('index.html', graph_html=graph_html)

if __name__ == '__main__':
    app.run(debug=True)
