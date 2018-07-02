import os
import flask
import requests
import flask_socketio
import flask_oauthlib
import flask_oauthlib.client

app = flask.Flask(__name__)

app.config['SECRET_KEY'] = 'secret!'
socketio = flask_socketio.SocketIO(app)


def setup_openid_connect():
    oauth = flask_oauthlib.client.OAuth()

    consumer_key = os.environ.get("OPENID_CONNECT_CONSUMER_KEY")
    consumer_secret = os.environ.get("OPENID_CONNECT_CONSUMER_SECRET")
    configuration_url = os.environ.get("OPENID_CONFIGURATION_URL")
    if not configuration_url:
        raise AssertionError("The 'OPENID_CONFIGURATION_URL' environment variable wasn't set!")

    try:
        openid_configuration_response = requests.get(configuration_url)
    except requests.RequestException as e:
        raise AssertionError(
            "Failed to retrieve the openid configuration from '%s'. The error was: %s" % (
                configuration_url, e))

    if openid_configuration_response.status_code != 200:
        raise AssertionError((
            "Failed to retrieve the openid configuration from '%s'. Got a %s response: %s" % (
                configuration_url,
                openid_configuration_response.status_code,
                openid_configuration_response.text)))

    try:
        openid_configuration = openid_configuration_response.json()
    except ValueError as e:
        raise AssertionError((
            "Failed to parse the openid configuration from '%s': %s\nresponse.text: %s" % (
                configuration_url,
                e,
                openid_configuration_response.text)))

    userinfo_endpoint = openid_configuration["userinfo_endpoint"]
    end_session_endpoint = openid_configuration.get("end_session_endpoint")

    oauth_remote_app = oauth.remote_app(
        "auth0",
        consumer_key=consumer_key,
        consumer_secret=consumer_secret,
        request_token_params={
            'scope': "openid profile email",
        },
        access_token_method="POST",
        access_token_url=openid_configuration["token_endpoint"],
        authorize_url=openid_configuration["authorization_endpoint"],
    )
    return oauth_remote_app, userinfo_endpoint, end_session_endpoint

#oauth_remote_app, userinfo_endpoint, end_session_endpoint = setup_openid_connect()
oauth_remote_app, userinfo_endpoint, end_session_endpoint = None, None, None

import controlcenter.views
