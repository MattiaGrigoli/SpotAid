from http.client import responses
from itertools import product
from django.contrib.auth import authenticate, login, logout
from django.http import HttpResponse
from django.shortcuts import render, redirect
from django.views.generic import TemplateView
from ServerApplication.models import *
from django.contrib import messages
import folium
from dataclasses import dataclass
import requests

# Create your views here.
def index(request):
    #return HttpResponse("Hello, world. You're at the polls index.")
    return render(request, "landing.html")

@dataclass
class SellingDataAnalysis:
    product_id: int
    percentage: float

# test map
class MapView(TemplateView):
    template_name = 'testMap.html'


    def get_context_data(self, **kwargs):

        figure = folium.Figure()
        target_url = "/ServerApplication"

        # Make the map
        map = folium.Map(
            location = [44.64551, 10.92530],
            zoom_start = 14,
            tiles = 'OpenStreetMap')

        map.add_to(figure)

        # Getting all the distributors
        all_distributors = Distributor.objects.all()

        # A list to fill with the operating distributors
        op_distributors = list()
        al_distributors = list()
        off_distributors = list()
        temp = list(all_distributors)

        # Adding a marker for each distributor
        for distributor in temp:

            popup_html = f'''
                <b>Coordinate:</b> 40.417, -3.70<br>
                <a href="{target_url}/{distributor.id}" >Clicca qui per la lista di prodotti!</a>
                <hr>
                dispenser all'interno del campus universitario DIEF UNIMORE
            '''

            if distributor.status == '1':   #Operative
                # Add a Marker
                folium.Marker(
                    location = [distributor.position_x, distributor.position_y],
                    popup = popup_html,
                    tooltip = distributor.address,
                    icon = folium.Icon(icon='fa-heart', prefix='fa', color='green')
                ).add_to(map)
                # Add distributor to the list
                op_distributors.append(distributor)

            #for the next two it is needed to find a way to hide them to un-authenticated users
            elif distributor.status == '2': #Alert
                # Add a Marker
                '''folium.Marker(
                    location=[distributor.position_x, distributor.position_y],
                    popup=popup_html,
                    tooltip=distributor.address,
                    icon=folium.Icon(icon='fa-heart', prefix='fa', color='red')
                ).add_to(map)'''
                # Add distributor to the list
                al_distributors.append(distributor)

            elif distributor.status == '3': #Offline
                # Add a Marker
                '''folium.Marker(
                    location=[distributor.position_x, distributor.position_y],
                    popup=popup_html,
                    tooltip=distributor.address,
                    icon=folium.Icon(icon='fa-heart', prefix='fa', color='gray')
                ).add_to(map)'''
                # Add distributor to the list
                off_distributors.append(distributor)

        all_products_in_distributors = ProductsInDistributor.objects.all()
        _temp = list(all_products_in_distributors)
        av_products = list()

        for pid in _temp:
            if pid.quantity > 0:
                av_products.append(pid)

        product_list = Product.objects.all()
        selling_list = Selling.objects.all()
        p_temp = list(product_list)
        s_temp = list(selling_list)
        selling_counter = selling_list.count()
        sda = list()    # to put the analyzed data from the sellings
        best_otm = None  # stands for "Best Seller Of The Month"

        for p in p_temp:
            counter = 0
            for s in s_temp:
                if s.date_time.month == datetime.now().month and p.id == s.id_product.id:
                    counter += 1

            if counter != 0:
                selling_data = SellingDataAnalysis(p.id, (counter / selling_counter)*100)
                sda.append(selling_data)
                if (best_otm is not None and selling_data.percentage > best_otm.percentage) or best_otm is None:
                    best_otm = selling_data

        # Render and send to template
        figure.render()
        return {"map": figure, "op_distributors": op_distributors, "products_inside": av_products, "product_list": product_list,
                "al_distributors": al_distributors, "off_distributors": off_distributors, "best_otm": best_otm}

# Used to get all the available products inside a specific distributor
'''def this_distributor(request, distributor_id):
    all_products_in_distributors = ProductsInDistributor.objects.all()
    _temp = list(all_products_in_distributors)
    av_products = list()    # To save the available products

    # Searches for all the available products (quantity>0) inside the specific distributor
    for pid in _temp:
        if pid.quantity > 0 and pid.id_distributor.id == distributor_id:
            av_products.append(Product.objects.get(id=pid.id_product.id))

    context = {"products_inside": av_products}
    return context'''

def loginPOST(request):
    if request.method == "POST":
        username = request.POST['username']
        password = request.POST['password']
        user = authenticate(request, username=username, password=password)
        if user is not None:
            login(request, user)
            return redirect('ServerApplication:testMap')

    else:
        messages.error(request, "wrong HTTP method")
        return render(request, "testMap.html")

def logoutGET(request):
    logout(request)
    return redirect('ServerApplication:testMap')

def listProduct(request, distributor_id):
    products_in_distributor = ProductsInDistributor.objects.select_related(
        'id_distributor',
        'id_product'
    ).filter(id_distributor=distributor_id).all()                           # Fixed
    products = Product.objects.all()

    context = {
        'prod_dist_list': products_in_distributor,
        'products': products,
        'id_distributor': distributor_id
    }
    return render(request, 'productList.html', context)

def updateCount(request):
    id_distributor = request.POST.get('id_distributor')
    id_product = request.POST.get('id_product')
    quantity = request.POST.get('quantity')
    item = ProductsInDistributor.objects.get(id_distributor=id_distributor, id_product=id_product)
    item.quantity = quantity
    item.save()
    sendCount(id_distributor)
    response = listProduct(request, id_distributor)
    return response

def removeCount(request):
    id_distributor = request.POST.get('id_distributor')
    id_product = request.POST.get('id_product')
    item = ProductsInDistributor.objects.get(id_distributor=id_distributor, id_product=id_product)
    item.delete()
    sendCount(id_distributor)
    response = listProduct(request, id_distributor)
    return response

def addCount(request):
    id_distributor = request.POST.get('id_distributor')
    id_product = request.POST.get('product')
    quantity = request.POST.get('quantity')
    distributor = Distributor.objects.get(id=id_distributor)
    product = Product.objects.get(id=id_product)
    item = ProductsInDistributor(id_distributor=distributor, id_product=product, quantity=quantity)
    item.save()
    sendCount(id_distributor)
    response = listProduct(request, id_distributor)
    return response

def sendCount(id_distributor):
    list = ProductsInDistributor.objects.filter(id_distributor=id_distributor)
    data = SerProductsInDistributor(list, many=True).data
    url_bridge = "http://192.168.1.190:8000/" #change for demonstration?

    try:
        response = requests.post(url_bridge, json=data)
        return
    except Exception as e:
        print(e)
        return False