from django.urls import path, include
from . import views
from . import api
from rest_framework.routers import DefaultRouter

from .models import Distributor

router = DefaultRouter()
router.register(r'distributor', api.DistributorViewSet)
router.register(r'product', api.ProductViewSet)
router.register(r'user', api.UserViewSet)
router.register(r'productsInDistributor', api.ProductsInDistributorViewSet)
router.register(r'selling', api.SellingViewSet)

urlpatterns = [
    path('', views.index, name='index'),
    path('testMap', views.MapView.as_view(), name='testMap'),
    path('login', views.loginPOST, name='loginPOST'),
    path('logoutGET', views.logoutGET, name='logoutGET'),
    #path('api/', api.indexAPI, name='indexAPI'),
    path('api/', include(router.urls)),
    path('<int:distributor_id>', views.listProduct, name = 'List'),
    path('updateCount', views.updateCount, name = 'updateCount'),
    path('removeCount', views.removeCount, name = 'removeCount'),
    path('addCount', views.addCount, name = 'addCount'),
]