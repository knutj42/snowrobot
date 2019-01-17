import flask
from controlcenter import app


@app.route('/')
def index():
    return flask.render_template("index.html")


@app.route('/config')
def config():
    return flask.render_template("index.html")
