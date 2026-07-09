function doPost(e) {
  var data = JSON.parse(e.postData.contents);

  
  Logger.log(JSON.stringify(data));
  
  var row = [
    data.time,
    data.temp1, data.hum1, data.pres1,
    data.temp2, data.hum2, data.pres2,
    data.temp3, data.hum3, data.pres3,
    data.v1, data.v2, data.mass, data.amp
  ];

  Logger.log(row);

  SpreadsheetApp.getActiveSpreadsheet()
    .getSheetByName("Data")
    .appendRow(row);

  return ContentService.createTextOutput("Success");
}
