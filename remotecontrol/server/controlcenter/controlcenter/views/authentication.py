import logging
import pprint
import textwrap
import urllib.parse

import flask
import flask_oauthlib.client
import jwt as jwt
import requests
from werkzeug.exceptions import BadRequest, Forbidden, ServiceUnavailable

from .. import app, oauth_remote_app, userinfo_endpoint, end_session_endpoint

log = logging.getLogger(__name__)


@app.route('/login')
def login():
    """This is called when a user wants to authenticate. This will redirect the user to the authentication
    provider. Once the user has logged it, they will be redirected back to this site, and the login_callback()
    function will be called.
    """
    flask.session.clear()
    callback_url = urllib.parse.urljoin(flask.request.host_url, "/login_callback")
    response = oauth_remote_app.authorize(callback=callback_url)
    return response


@app.route('/login_callback')
def login_callback():
    """This is called when a user has attempted to authenticate with this provider and
    the provider has redirected the user back to the our site (the request url will
    contain the results of the authentication-attempt)."""
    request = flask.request

    try:
        resp = oauth_remote_app.authorized_response()
    except flask_oauthlib.client.OAuthException as e:
        raise BadRequest("Login failed, please try again.")

    if resp is None:
        error = request.args.get('error')
        error_description = request.args.get('error_description')
        if error is None and error_description is None:
            # this usually means that the user cancelled the login, so just redirect to the front page
            return flask.redirect(flask.request.host_url)

        raise Forbidden('Access denied: error=%s error_description=%s' % (
            request.args.get('error'),
            request.args.get('error_description')
        ))

    id_token_payload = jwt.decode(resp["id_token"], verify=False, leeway=3600,
                                  options={"verify_aud": False})

    # remember the id_token, since this is required by the logout() method.
    flask.session["authentication_provider_id_token"] = resp["id_token"]

    access_token = resp.get('access_token')
    nonce = id_token_payload.get("nonce")
    if nonce is not None:
        # The id_token contains a nonce, so check if it is still valid.
        expected_nonce = flask.session.pop("authentication_provider_nonce", None)
        if nonce != expected_nonce:
            raise BadRequest("Login failed (invalid nonce), please try again.")

    # Ee use the user-info endpoint to get additional information about
    # the user.
    headers = {'authorization': 'Bearer ' + access_token}
    userinfo_endpoint_response = requests.get(userinfo_endpoint, headers=headers)
    if userinfo_endpoint_response.status_code != 200:
        log.error("The authentication attempt failed, since we failed to call the userinfo endpoint at '%s'.\n"
                  "  resp.status_code: %s\n"
                  "  resp.text: %s",
                  userinfo_endpoint, userinfo_endpoint_response.status_code, userinfo_endpoint_response.text)
        raise ServiceUnavailable("The authentication attempt failed. Please try again later.")
    user_info = userinfo_endpoint_response.json()

    session = flask.session
    session["is_authenticated"] = True
    session["user_id"] = user_info["sub"]
    session["user_info"] = user_info

    log.info("A user logged in. user_info:\n%s" % (
        textwrap.indent(pprint.pformat(user_info), prefix="    ")
    ))
    # All went well, so redirect to the frontpage
    return flask.redirect(flask.request.host_url)


@app.route('/logout')
def logout():
    """
    This is called when the user has clicked the "logout"-button
    """
    user_info = flask.session.get("user_info")
    if user_info:
        log.info("A user logged logged out. user_info:\n%s" % (
            textwrap.indent(pprint.pformat(user_info), prefix="    ")
        ))

    flask.session.clear()
    if end_session_endpoint:
        # the authentication provider supports logout, so redirect to it.
        urlparams = {
            "post_logout_redirect_uri": flask.request.host_url,
        }
        id_token_hint = flask.session.get("authentication_provider_id_token")
        if id_token_hint:
            urlparams["id_token_hint"] = id_token_hint
        logout_url = end_session_endpoint + "?" + urllib.parse.urlencode(urlparams)
        return flask.redirect(logout_url)
    else:
        # The authentication provider doesn't support logout, so just redirect to the frontpage
        return flask.redirect(flask.request.host_url)
