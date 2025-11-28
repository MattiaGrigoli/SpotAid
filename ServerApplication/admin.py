from django.contrib import admin
from . import models

# Register your models here.
admin.site.register(models.Distributor)
admin.site.register(models.Product)
admin.site.register(models.ProductsInDistributor)
admin.site.register(models.User)