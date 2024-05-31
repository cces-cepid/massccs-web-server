#!/bin/sh
#./add_cron_job.sh
#* * * * * cd /massccs-web-server/run/ ; ./clean.sh
#cron -f &
#cron && tail -f /var/log/cron.log &
cron -l 2 -f &
python manage.py makemigrations
python manage.py migrate
python manage.py runserver 0.0.0.0:8000