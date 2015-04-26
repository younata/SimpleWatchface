var xhrRequest = function (url, type, tries, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    if (xhr.status == 200) {
      callback(this.responseText); 
    } else {
      console.log('Error requesting weather: ' + this.responseText);
      if (tries != 0) {
        console.log('Trying again with ' + tries + ' tries left');
        xhrRequest(url, type, tries - 1, callback);
      }
    }
  };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(position) {
  var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' + 
      position.coords.latitude + '&lon=' + position.coords.longitude;
  
  xhrRequest(url, 'GET', 2, 
    function(responseText) {
      var json = JSON.parse(responseText);

      var temperature = ((9.0 / 5.0) * Math.round(json.main.temp - 273.15)) + 32;

      // Conditions
      var conditions = json.weather[0].main;
      
      var dictionary = {
        'KEY_TEMPERATURE': temperature,
        'KEY_CONDITIONS': conditions
      };
      
      Pebble.sendAppMessage(dictionary);
    }      
  );
}

function locationError(error) {
  console.log('Error requesting location: ' + error);
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout:15000, maximumAge: 60000}
  );
}

Pebble.addEventListener('ready', function(event) {
  getWeather();
});

Pebble.addEventListener('appmessage', function(event) {
  getWeather();
});