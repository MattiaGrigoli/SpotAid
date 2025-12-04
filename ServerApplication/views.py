from django.http import HttpResponse
from django.shortcuts import render
from django.views.generic import TemplateView
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
            dispenser allìinterno del campus universitario DIEF UNIMORE
        '''

        # Make the map
        map = folium.Map(
            location=[44.64551, 10.92530],
            zoom_start=14,
            tiles='OpenStreetMap')

        map.add_to(figure)

        # Add a Marker
        folium.Marker(
            location=[44.629335, 10.948308],
            popup=popup_html,
            tooltip='dispenser DIEF',
            icon=folium.Icon(icon='fa-heart', prefix='fa', color='green')
        ).add_to(map)

        # Render and send to template
        figure.render()
        return {"map": figure}