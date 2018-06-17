# This creates a ssh-tunnel to the robots.knutj.org server. The idea is to route the traffic to the webserver to
# local instance of the webserver that is running in a debugger.
set -e

# switch to using the nginx config that forwards the traffic to the ssh tunnel
ssh knutj@robots.knutj.org "rm -f nginx_default.conf && ln -s nginx_default.conf.ssh_tunnel nginx_default.conf && docker-compose restart nginx"

set +e

# Create an ssh tunnel from 15000=>16000. We assume that the react dev server has been started with the "run_webapp.sh"
# script. We also create a tunnel for the vannpumpe server on port 17000. TODO: clean up this a bit;
# the various servers should have separate nginx config files.

ssh -R 15000:0.0.0.0:16000 -R 17000:0.0.0.0:17000 knutj@robots.knutj.org



# restore the production nginx config
ssh knutj@robots.knutj.org "rm -f nginx_default.conf && ln -s nginx_default.conf.prod nginx_default.conf && docker-compose restart nginx"
