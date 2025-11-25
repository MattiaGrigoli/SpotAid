from django.core.management.commands.makemessages import STATUS_OK
from django.db import models


# Create your models here.
class Distributor(models.Model):
    class Status(models.TextChoices):
        OPERATING = 1, 'Operating'
        ALERT = 2, 'Alert'
        OFFLINE = 3, 'Offline'

    id = models.IntegerField(primary_key=True, auto_created=True, unique=True)
    status = models.CharField(max_length=200, choices=Status.choices)
    #temporary
    address = models.CharField(max_length=200) #the address and...
    position = models.CharField(max_lenght = 200) #the geolocalization of the distributor
##

class Product(models.Model):
    id = models.AutoField(primary_key=True, unique=True)
    name = models.CharField(max_length=200)
    price = models.DecimalField(max_digits=10, decimal_places=2)
##

#table for the relationship between the tables Distributor and Product.
# it also contains the number of the specific product in each distributor
class ProductsInDistributor(models.Model):
    id_distributor = models.ForeignKey(Distributor, on_delete=models.CASCADE, null = False)
    id_product = models.ForeignKey(Product, null = False)
    numberOfProducts = models.IntegerField()
    models.CompositePrimaryKey('id_distributor', 'id_product')