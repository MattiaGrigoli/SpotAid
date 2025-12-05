from django.urls import path, include
from . import views
from . import api
from rest_framework.routers import DefaultRouter

router = DefaultRouter()
router.register(r'distributor', api.DistributorViewSet)
router.register(r'product', api.ProductViewSet)
router.register(r'user', api.UserViewSet)
router.register(r'productsInDistributor', api.ProductsInDistributorViewSet)

urlpatterns = [
    path('', views.index, name='index'),
    path('testMap', views.MapView.as_view(), name='testMap'),
    #path('api/', api.indexAPI, name='indexAPI'),
    path('api/', include(router.urls)),
]