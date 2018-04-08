import flask_socketio
import flask
import logging
from controlcenter import socketio

logger = logging.getLogger(__name__)


@socketio.on('message')
def handle_message(message):
    logger.info("Received message: %s", message)
    flask_socketio.emit("message", message, broadcast=True)


@socketio.on('json')
def handle_json(json_msg):
    logger.info('received json: %s', json_msg)
    flask_socketio.emit("message", json_msg, broadcast=True)


@socketio.on('create or join')
def handle_create_or_join(room):
    logger.info('Received request to create or join room ' + room)

    rooms = socketio.server.manager.rooms
    room_obj = rooms["/"].get(room)
    if room_obj is None:
        numClients = 0
    else:
        numClients = len(room_obj)

    if numClients == 0:
        flask_socketio.join_room(room)
        logger.info('Client ID ' + flask.request.sid + ' created room ' + room)
        flask_socketio.emit('created', room)

    elif numClients == 1:
        logger.info('Client ID ' + flask.request.sid + ' joined room ' + room)
        flask_socketio.emit('join', room, room=room)
        flask_socketio.join_room(room)
        flask_socketio.emit('joined', room)
        flask_socketio.emit('ready', room=room)
    else: # max two clients
        flask_socketio.emit('full', room)


@socketio.on('ipaddr')
def handle_ipaddr():
    logger.warning("handle_ipaddr() running")
    raise NotImplementedError()
    """
    var ifaces = os.networkInterfaces();
    for (var dev in ifaces) {
      ifaces[dev].forEach(function(details) {
        if (details.family === 'IPv4' && details.address !== '127.0.0.1') {
          socket.emit('ipaddr', details.address);
        }
      });
    }
  });"""


@socketio.on('connect')
def test_connect():
    logger.info("Client connected")


@socketio.on('disconnect')
def test_disconnect():
    logger.info('Client disconnected')


@socketio.on('my event')
def handle_my_custom_event(json_msg):
    logger.info('received my custom event: %s', json_msg)

    flask_socketio.emit('my response', json_msg)
