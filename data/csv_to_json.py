import json
import csv

#CSV file path
csvfile = "C:/Users/karam/Desktop/values.csv"

with open(csvfile, "r" ) as f:
    reader = csv.reader(f)
    next(reader)
    data = {"data":[]}
    for row in reader:
        data["data"].append({"Timestamp":row[0],
        "UTC_offset":"" ,
        "longitude":"",
        "latitude":"",
        "sensors":{"Temperatue":row[1],
                   "Temperature_unit":"C",
                   "ph":row[2],
                   "water_conductivity": "",
                   "water_conductivity_unit":""}})

jsonfile = json.dumps(data)
print(jsonfile)
