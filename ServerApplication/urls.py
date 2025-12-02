from django.urls import path
from . import views
from . import api

urlpatterns = [
    path('', views.index, name='index'),
    path('testMap', views.MapView.as_view(), name='testMap'),
    path('api/', api.indexAPI, name='indexAPI'),
]