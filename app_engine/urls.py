from django.urls import path
from . import views

urlpatterns = [
  path('',views.home),
  path('configuration/',views.configuration, name='configuration'),
  path('result',views.result, name='result'), 
  path('download/<slug:job_id>', views.logfile, name='logfile'),
  path('about/', views.about),
  path('doc/', views.doc),
]