FROM python

ENV PYTHONDONTWRITEBYTECODE 1
ENV PYTHONUNBUFFERED 1

WORKDIR /massccs-web-server

USER root

RUN apt-get update && apt-get install -y gcc && apt-get install -y cmake && apt-get install -y cron
    

COPY requirements.txt /massccs-web-server/
RUN pip install -r requirements.txt

COPY . /massccs-web-server/

# Add crontab file
COPY cronfile /var/spool/cron/crontabs/root
RUN chmod +x /massccs-web-server/cleanup_files.py

# COPY cronfile /etc/cron.d/cronfile
# Give execution rights on the cron job
# RUN chmod 0644 /etc/cron.d/cronfile
# Create the log file to be able to run tail
# RUN crontab /etc/cron.d/cronfile
# RUN touch /var/log/cron.log

#COPY crontab.txt /opt
#RUN crontab -l > /opt/crontab.txt
#RUN crontab /opt/crontab.txt
#RUN chmod +x clean.sh
RUN chmod +x django.sh
RUN chmod +x massccs.sh
RUN /massccs-web-server/massccs.sh

EXPOSE 8000

CMD ["./django.sh"]
