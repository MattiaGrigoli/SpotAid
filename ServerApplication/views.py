from django.contrib.auth import authenticate, login, logout
from django.http import HttpResponse
from django.shortcuts import render, redirect
from django.views.generic import TemplateView
from ServerApplication.models import *
from django.contrib import messages
import folium


# Create your views here.
def index(request):
    #return HttpResponse("Hello, world. You're at the polls index.")
    return render(request, "landing.html")

# test map
class MapView(TemplateView):
    template_name = 'testMap.html'


    def get_context_data(self, **kwargs):

        figure = folium.Figure()
        target_url = "/ServerApplication"
        popup_html = f'''
            <b>Coordinate:</b> 40.417, -3.70<br>
            <a href="{target_url}" target="_blank">Clicca qui per la lista di prodotti!</a>
            <hr>
            dispenser all'interno del campus universitario DIEF UNIMORE
        '''

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
        temp = list(all_distributors)

        # Adding a marker for each distributor
        for distributor in temp:
            if distributor.status == '1':
                # Add a Marker
                folium.Marker(
                    location = [distributor.position_x, distributor.position_y],
                    popup = popup_html,
                    tooltip = distributor.address,
                    icon = folium.Icon(icon='fa-heart', prefix='fa', color='green')
                ).add_to(map)
                # Add distributor to the list
                op_distributors.append(distributor)

        all_products_in_distributors = ProductsInDistributor.objects.all()
        _temp = list(all_products_in_distributors)
        av_products = list()
        for pid in _temp:
            if pid.quantity > 0:
                av_products.append(pid)

        product_list = Product.objects.all()
        # Render and send to template
        figure.render()
        return {"map": figure, "op_distributors": op_distributors, "products_inside": av_products, "product_list": product_list}

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