#this file is for managing all the API side of the server, no browser client

from django.http import HttpResponse
from rest_framework import serializers # pip install djangorestframework

def indexAPI (request):
    return HttpResponse("Hello, world. You're at the API index") #change in serializer in the future, this just for test purposes

# all other methods