
import logging

import flask
from werkzeug.exceptions import BadRequest

from . import viewutils
from .pyramid_to_flask_conversion_utils import FlaskRequestWrapper
from .. import config_module
from .. import models

log = logging.getLogger(__name__)


def logout():
    """
    This is called when the user has clicked the "logout"-button
    """
    config = config_module.get_config()
    provider_id = flask.session.get("authentication_provider_id")
    if provider_id:
        # the user was authenticated with some external authentication provider, so we must tell the
        # provider that the user wants to log out.
        provider = config.get_authentication_provider(provider_id)
        if provider is not None:
            response = provider.logout()
            if response is not None:
                flask.session.clear()
                return response

    flask.session.clear()
    pyramid_request = FlaskRequestWrapper()
    application_url = viewutils.get_real_virtualhost_aware_url(pyramid_request, pyramid_request.application_url)
    return flask.redirect(application_url)


def login(provider_id):
    flask.session.clear()

    config = config_module.get_config()
    provider = config.get_authentication_provider(provider_id)
    if provider is None:
        raise BadRequest("The authentication provider '%s' does not exist!" % (provider_id,))

    response = provider.authorize()

    return response


def login_callback(provider_id):
    config = config_module.get_config()
    provider = config.get_authentication_provider(provider_id)
    if provider is None:
        raise BadRequest("The authentication provider '%s' does not exist!" % (provider_id,))

    user_info = provider.authorized_response()
    session = flask.session
    session['authentication_provider_id'] = provider_id
    session["is_authenticated"] = True
    session["user_id"] = user_info["user_id"]

    userobject = None
    psis = None
    email = ""
    if "email" in user_info:
        # This is an email-based authentication provider, where the email-adress is the user_id, and
        # we can do a lookup in the solr-data based on that email address.
        if "email_verified" not in user_info and not provider.allow_unverified_email:
            raise AssertionError(
                ("The user_info from the authentication provider '%s' didn't contain a 'email_verified' "
                 "property and the 'allow_unverified_email'-property is 'False'! user_info.keys(): %s") % (
                    provider_id, list(user_info.keys())
                ))
        if user_info.get("email_verified") or provider.allow_unverified_email:
            email = user_info.get("email")
            email = email.strip()
        if email:
            userobject, psis = models.perform_email_search(email)
            session['email'] = email
            session["user_id"] = email
    else:
        # This is not an email-based auth provider (it could be for instance BankID), so we don't have
        # a way to look up the user's data in solr. Currently, we only use such auth providers in the
        # gdpr platform, where the data in solr is tied to a data accessrequest instead of directly to
        # the user. If we want to be able to look up the user-data in solr directly based on the
        # user_id, we need to introduce a new "USER_ID_FIELDS" config-setting and related search-functionality
        # (similar to the EMAIL_FIELDS config-setting works for email-based users today).
        pass

    if psis and len(psis) > 0:
        session['userobject'] = userobject
        session['psis'] = psis

    # All went well, so redirect to the frontpage
    pyramid_request = FlaskRequestWrapper()
    application_url = viewutils.get_real_virtualhost_aware_url(pyramid_request, pyramid_request.application_url)
    return flask.redirect(application_url)
