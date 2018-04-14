import flask_socketio
import flask
import logging
from controlcenter import socketio

logger = logging.getLogger(__name__)

# The sid of the robot
robot_sid = None

# The sid of the controller webapp
controller_sid = None

@socketio.on('i-am-the-robot')
def handle_i_am_the_robot():
    """This handles the initial message that the robot sends when it connects to the server."""
    global robot_sid, controller_sid
    logger.info("handle_i_am_the_robot(): old robot_sid:%s  new robot_sid:%s",
                robot_sid, flask.request.sid)
    if robot_sid != flask.request.sid:
        old_robot_sid = robot_sid
        robot_sid = flask.request.sid
        if old_robot_sid:
            logger.warning("handle_i_am_the_robot(): disconnecting the old robot_sid:%s  (new robot_sid:%s)",
                           old_robot_sid, flask.request.sid)
            flask_socketio.disconnect(sid=old_robot_sid)
        if controller_sid:
            # We already have a controller, so tell the robot and controller about each other
            flask_socketio.emit("robot-connected", room=controller_sid)
            flask_socketio.emit("controller-connected", room=robot_sid)


@socketio.on('i-am-the-controller')
def handle_i_am_the_controller():
    """This handles the initial message that the controller webapp sends when it connects to the server."""
    global robot_sid, controller_sid
    logger.info("handle_i_am_the_controller(): old controller_sid:%s  new controller_sid:%s",
                controller_sid, flask.request.sid)
    if controller_sid != flask.request.sid:
        old_controller_sid = controller_sid
        controller_sid = flask.request.sid
        if old_controller_sid:
            logger.warning("handle_i_am_the_controller(): disconnecting the old controller_sid:%s  (new controller_sid:%s)",
                           old_controller_sid, flask.request.sid)
            flask_socketio.disconnect(sid=old_controller_sid)

        if robot_sid:
            # We already have a robot_sid, so tell it that the controller has connected
            flask_socketio.emit("controller-connected", room=robot_sid)
            flask_socketio.emit("robot-connected", room=controller_sid)


@socketio.on('webrtc-offer')
def handle_webrtc_offer(message):
    """This handles the message the controller webapp sends when it wants to connect to the robot."""
    global controller_sid, robot_sid
    logger.info("handle_webrtc_offer(): %s", message)
    if flask.request.sid != controller_sid:
        logger.warning("Got a 'webrtc-offer' -message from a sid '%s' that wasn't the controller_sid ('%s')!", 
                       flask.request.sid, controller_sid)
        flask_socketio.disconnect(sid=flask.request.sid)
        return

    if not robot_sid:
        logger.warning("Got a 'webrtc-offer' -message, but no robot is currently connected!",
                       flask.request.sid, controller_sid)
        flask_socketio.disconnect(sid=flask.request.sid)
        return

    # send the offer to the robot
    flask_socketio.emit('webrtc-offer', message, room=robot_sid)


@socketio.on('webrtc-answer')
def handle_webrtc_answer(message):
    """This handles the message the robot sends when it wants to accept the 'webrtc-offer' request from
    the controller app."""
    global controller_sid, robot_sid
    logger.info("handle_webrtc_answer(): %s", message)
    if flask.request.sid != robot_sid:
        logger.warning("Got a 'webrtc-answer' -message from a sid '%s' that wasn't the robot_sid ('%s')!",
                       flask.request.sid, robot_sid)
        flask_socketio.disconnect(sid=flask.request.sid)
        return

    if not controller_sid:
        logger.warning("Got a 'webrtc-answer' -message, but no controller is currently connected!",
                       flask.request.sid, controller_sid)
        flask_socketio.disconnect(sid=flask.request.sid)
        return

    # send the answer to the controller
    socketio.emit('webrtc-answer', message, room=controller_sid)


@socketio.on('ice-candidate')
def handle_ice_candidate(message):
    """This handles the message the robot and controller sends when it tell the other party about
    a connection candidate."""
    global controller_sid, robot_sid
    logger.info("handle_ice_candidate(): sid:%s %s", flask.request.sid, message)
    if flask.request.sid == robot_sid:
        # forward the candiate to the controller
        flask_socketio.emit('ice-candidate', message, room=controller_sid)

    elif flask.request.sid == controller_sid:
        # forward the candiate to the robot
        flask_socketio.emit('ice-candidate', message, room=robot_sid)

    else:
        logger.warning(
            "Got a 'ice-candidate'-message from a client (sid:'%s') that is neither the robot nor the controller!",
            flask.request.sid)
        flask_socketio.disconnect(sid=flask.request.sid)
        return


@socketio.on('connect')
def test_connect():
    logger.info("Client connected")


@socketio.on('disconnect')
def test_disconnect():
    global robot_sid, controller_sid
    logger.info('Client %s disconnected', flask.request.sid)
    if flask.request.sid == robot_sid:
        logger.info("The robot %s disconnected.", robot_sid)
        robot_sid = None
        if controller_sid:
            flask_socketio.emit("robot-disconnected", room=controller_sid)

    if flask.request.sid == controller_sid:
        logger.info("The controller %s disconnected.", controller_sid)
        controller_sid = None
        if robot_sid:
            flask_socketio.emit("controller-disconnected", room=robot_sid)
