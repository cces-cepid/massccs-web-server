from django.db import models

BUFFER_GAS = (
  ('Helium', 'He'),
  ('Nitrogen', 'N2 '),
)

# Create your models here.
class InformationMASSCCS(models.Model):
  temperature = models.DecimalField(max_digits=5, decimal_places=2, default=298.0)
  seed = models.IntegerField(default=2104)
  gas = models.CharField(max_length=8, choices=BUFFER_GAS, default='Helium')
  ccs_avg = models.FloatField()
  ccs_err = models.FloatField()  
  time_execution = models.FloatField()
  job_id = models.CharField(max_length=100)
  date = models.DateField(auto_now_add=True)
  ip_client = models.CharField(max_length=100)
  successful = models.CharField(max_length=10)
  err = models.CharField(max_length=255)
  status_ip = models.CharField(max_length=10)
  country_ip = models.CharField(max_length=32)
  country_code_ip = models.CharField(max_length=10)
  city_ip = models.CharField(max_length=32)