#this file is for managing all the API side of the server, no browser client

from django.http import HttpResponse
from rest_framework import serializers, viewsets  # pip install djangorestframework
from .models import *

def indexAPI (request):
    return HttpResponse("Hello, world. You're at the API index") #change in serializer in the future, this just for test purposes

# all other methods
class DistributorViewSet(viewsets.ModelViewSet):
    queryset = Distributor.objects.all()
    serializer_class = SerDistributor

class UserViewSet(viewsets.ModelViewSet):
    queryset = User.objects.all()
    serializer_class = SerUser

class ProductViewSet(viewsets.ModelViewSet):
    queryset = Product.objects.all()
    serializer_class = SerProduct

class ProductInDistributorViewSet(viewsets.ModelViewSet):
    queryset = ProductsInDistributor.objects.all()
    serializer_class = SerProductInDistributor