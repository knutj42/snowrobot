import flask
import flask_socketio


app = flask.Flask(__name__)

app.config['SECRET_KEY'] = 'secret!'
socketio = flask_socketio.SocketIO(app)

import controlcenter.views
