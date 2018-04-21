import flask_socketio
import logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s %(name)-12s %(threadName)-10s %(levelname)-8s %(message)s')

from controlcenter import app, socketio

logger = logging.getLogger(__name__)
if __name__ == '__main__':
    host = "0.0.0.0"
    port = 15000
    logger.info("Starting webserver at http://%s:%s" % (host, port))
    socketio.run(app, host=host, port=port)
