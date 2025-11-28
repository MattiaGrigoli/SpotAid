from django.core.management.commands.makemessages import STATUS_OK
from django.db import models
from rest_framework import serializers

# Create your models here.
class Distributor(models.Model):
    class Status(models.TextChoices):
        OPERATING = '1', 'Operating'
        ALERT = '2', 'Alert'
        OFFLINE = '3', 'Offline'

    id = models.AutoField(primary_key=True, unique=True)
    status = models.CharField(max_length=200, choices=Status.choices)
    #temporary
    address = models.CharField(max_length=200) #the address and...
    position = models.CharField(max_length = 200) #the geolocalization of the distributor

    def __str__(self):
        return ('\nID: ' + str(self.id) + '\nStatus: ' + self.status + '\nAddress: ' + self.address +
                '\nPosition: ' + self.position)

    def to_dict(self):
        address = self.address if self.address else None
        position = self.position if self.position else None
        return {'id': self.id, 'status': self.status, 'address': address, 'position': self.position}
##

class Product(models.Model):
    id = models.AutoField(primary_key=True, unique=True)
    name = models.CharField(max_length=200)
    price = models.DecimalField(max_digits=10, decimal_places=2)

    def to_dict(self):
        return {'id': self.id, 'name': self.name, 'price': self.price}
##

#table for the relationship between the tables Distributor and Product.
# it also contains the number of the specific product in each distributor
class ProductsInDistributor(models.Model):
    id_distributor = models.ForeignKey(Distributor, on_delete = models.CASCADE, null = False)
    id_product = models.ForeignKey(Product, on_delete = models.CASCADE, null = False)
    quantity = models.IntegerField()

    def to_dict(self):
        quantity = self.quantity if self.quantity > 0 else 0
        return {'id_distributor': self.id_distributor.id, 'id_product': self.id_product.id, 'quantity': quantity}

# if exists then pharmacy, otherwise normal user
class User(models.Model):
    id = models.AutoField(primary_key=True)
    name = models.CharField(max_length=200)
    email = models.EmailField()
    password = models.CharField(max_length=200)

    def to_dict(self):
        return {'id': self.id, 'name': self.name, 'email': self.email, 'password': self.password}

#Serializers for JSON response in API
class SerDistributor(serializers.ModelSerializer):
    class Meta:
        model = Distributor
        fields = "__all__"

class SerProduct(serializers.ModelSerializer):
    class Meta:
        model = Product
        fields = "__all__"

class SerUser(serializers.ModelSerializer):
    class Meta:
        model = User
        fields = "__all__"
class SerProductInDistributor(serializers.ModelSerializer):
    class Meta:
        model = ProductsInDistributor
        fields = "__all__"