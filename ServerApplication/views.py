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
            <a href="{target_url}" target="_blank">Clicca qui per i dettagli!</a>
            <hr>
            Integrazione di folium con django
        '''

        # Make the map
        map = folium.Map(
            location=[40.416, -3.70],
            zoom_start=11,
            tiles='OpenStreetMap')

        map.add_to(figure)

        # Add a Marker
        folium.Marker(
            location=[40.417, -3.70],
            popup=popup_html,
            tooltip='folium and django',
            icon=folium.Icon(icon='fa-coffee', prefix='fa')
        ).add_to(map)

        # Render and send to template
        figure.render()
        return {"map": figure}