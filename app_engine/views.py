from django.contrib import messages
from django.shortcuts import render, redirect
#from django.shortcuts import render
from django.http import HttpResponse
import json
import uuid
import os
import threading
from queue import Queue
from django.utils import timezone
from . import models
import requests
from django.views.decorators.csrf import csrf_protect

request_queue = Queue()
lock = threading.Lock()

def configuration(request):

  ip_client = request.META.get('HTTP_X_FORWARDED_FOR') or request.META.get('REMOTE_ADDR')

  # parameters to retrieve from API
  params = ['query', 'status', 'country', 'countryCode', 'city']

  # make the response 
  resp = requests.get('http://ip-api.com/json/' + ip_client, params={'fields': ','.join(params)})

  # read response as JSON (converts to Python dict)
  info_ip_client = resp.json() 

  if request.method == 'POST':
    
    uploadFile = request.FILES["moleculeFile"]  # input file of molecule in format XYZ or PQR
    bufferGas = request.POST["buffergas"]       #    
    temperature = request.POST["temperature"]
    seed = request.POST["seed"]
      
    fileName = uploadFile.name                  # input file name
    fileExtension = fileName.split(".")[-1]     # input file extension 
    
    job_id = str(uuid.uuid4()) [:8]             # generate unique id for the job 
    input_file = job_id + '.' + fileExtension
    input_path = 'run/' + input_file

    with open(input_path, 'wb') as f:
      for chunk in uploadFile.chunks():
        f.write(chunk) 

    input_data = {
      "targetFileName": input_path,
      "numberProbe" : 10000,
      "nIter" : 10,
      "seed" : int(seed),
      "dt" : 10.0,
      "Temp" : float(temperature),
      "skin" : 0.01,
      "GasBuffer" : bufferGas,
      "Equipotential" : "no",
      "Short-range cutoff" : "yes",
      "LJ-cutoff" : 12.0,
      "Long-range forces" : "yes",
      "Long-range cutoff" : "yes",
      "Coul-cutoff" : 25.0,
      "polarizability" : "yes"
    } 

    json_data = json.dumps(input_data)

    input_json = job_id + '.json'
    input_path = 'run/' + input_json 

    output = job_id + '.log'
    output_path = 'run/' + output  
    
    with open(input_path, 'w') as f:
      f.write(json_data) 

    with lock:
      command = './run/massccs run/' + input_json + ' > run/' + output + ' 2>&1'
      status_code = os.system(command)

      logfile = open(output_path, 'r')
      loglines = logfile.readlines()

      if status_code == 0:
        for line in loglines:
          if line.find('average') != -1:
            ccs_avg = float(line.split()[5])                                                
          if line.find('error') != -1:
            ccs_err = float(line.split()[5])
          if line.find('Total time') != -1:
            time_exec = float(line.split()[2])
      else:
        for line in loglines:
          if line.find('only') != -1:
            err = line
          if line.find('what') != -1:
            words = line.split()
            words_without_word_to_exclude = [word for word in words if word != 'what():']
            err = ' '.join(words_without_word_to_exclude)            

      logfile.close()

    if status_code == 0:
      # save database     
      if info_ip_client['status'] == 'success':
        information = models.InformationMASSCCS(temperature=temperature, seed=seed, gas=bufferGas, ccs_avg=ccs_avg, ccs_err=ccs_err, 
          time_execution=time_exec, job_id=job_id, ip_client=ip_client, successful='yes', status_ip=info_ip_client['status'], 
          country_ip=info_ip_client['country'], country_code_ip=info_ip_client['countryCode'], city_ip=info_ip_client['city'])      
      else:
        information = models.InformationMASSCCS(temperature=temperature, seed=seed, gas=bufferGas, ccs_avg=ccs_avg, ccs_err=ccs_err, 
          time_execution=time_exec, job_id=job_id, ip_client=ip_client, successful='yes', status_ip=info_ip_client['status'])        
      information.save()
      return render(request, "result.html", {'ccs': ccs_avg, 'err': ccs_err, 'time': time_exec, 'success': 1, 'jobid': job_id})
    else:
      # save database
      if info_ip_client['status'] == 'success':
        information = models.InformationMASSCCS(temperature=temperature, seed=seed, gas=bufferGas, job_id=job_id, ip_client=ip_client, 
          successful='no', err=err, status_ip=info_ip_client['status'], country_ip=info_ip_client['country'],
          country_code_ip=info_ip_client['contryCode'], city_ip=info_ip_client['city'])
      else:
        information = models.InformationMASSCCS(temperature=temperature, seed=seed, gas=bufferGas, job_id=job_id, ip_client=ip_client, 
          successful='no', err=err, status_ip=info_ip_client['status'])
      information.save()
      return render(request, "result.html", {'err': err, 'success': 0, 'jobid': job_id})
  return render(request,'configuration.html')

def logfile(request, job_id):

  logfile = job_id + '.log' # example: b17168d0.log
  logfile_path = 'run/' + logfile  # Path to the generated file example: run/b17168d0.log
  
  if os.path.exists(logfile_path):

    log = open(logfile_path, 'r')
    loglines = log.readlines()
    for line in loglines:
      if line.find('target filename') != -1:
        extension = line.split(".")[-1].strip()
    log.close()    

    molfile_path =  'run/' + job_id + '.' + extension
    jsonfile_path = 'run/' + job_id + '.json'

    with open(logfile_path, 'rb') as f:
      response = HttpResponse(f.read(), content_type='text/plain')
      response['Content-Disposition'] = 'attachment; filename="{}"'.format(logfile)
      # remove logfile
      # os.remove(logfile_path)
      # remove input molecule file
      # os.remove(molfile_path)
      # remove json file
      # os.remove(jsonfile_path)
    return response
  
  else:
    return render(request, "result.html", {'notfile': 1, 'text': logfile, 'jobid': job_id})
 
def home(request):
  return render(request, 'home.html')

def result():
  pass

def about(request):
  return render(request, 'about.html')

def doc(request):
  return render(request, 'doc.html')
