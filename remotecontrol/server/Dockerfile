FROM python:3

WORKDIR /usr/src/app
RUN pip install --upgrade pip
COPY ./controlcenter ./controlcenter
RUN pip install --no-cache-dir -r ./controlcenter/requirements.txt
RUN pip install -e ./controlcenter
COPY run_server.py .

CMD [ "python", "run_server.py" ]