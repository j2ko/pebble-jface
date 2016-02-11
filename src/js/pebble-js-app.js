Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('showConfiguration', function() {
  var url = 'http://j2ko.github.io/krangface-settings/';
  
  console.log('Showing configuration page: ' + url);

  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
  var configData = JSON.parse(decodeURIComponent(e.response));

  console.log('Configuration page returned: ' + JSON.stringify(configData));

  if (configData.backgroundColor) {
    Pebble.sendAppMessage({
      backgroundColor: parseInt(configData.backgroundColor, 16),
      timeColor: parseInt(configData.timeColor, 16),
      dateColor: parseInt(configData.dateColor, 16),
      twentyFourHourFormat: configData.twentyFourHourFormat
    }, function() {
      console.log('Send successful!');
    }, function() {
      console.log('Send failed!');
    });
  }
});
