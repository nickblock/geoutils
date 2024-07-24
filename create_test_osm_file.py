#!/usr/bin/env python3

from xml.etree.ElementTree import Element, ElementTree, SubElement
import xml.dom.minidom
import argparse


class OSMBuilder:

    def __init__(self):

        self.nodeIdx = 0
        self.wayIdx = 0
        self.buildingIdx = 0
        self.highwayIdx = 0

        self.top = Element("osm")
        self.top.set('generator', 'osm2assimp test generator')
        self.top.set('version', '0.6')
        self.bounds = SubElement(self.top, "bounds")

        self.tree = ElementTree(self.top)

        self.minLatLon = {}
        self.maxLatLon = {}


    def check_minmax(self, lonlat):

        if self.nodeIdx == 0:

            self.minLatLon = {
                "lat": lonlat["lat"],
                "lon": lonlat["lon"],
            }
            self.maxLatLon = {
                "lat": lonlat["lat"],
                "lon": lonlat["lon"],
            }

        else:

            if lonlat["lat"] < self.minLatLon["lat"]:
                self.minLatLon["lat"] = lonlat["lat"]
            if lonlat["lon"] < self.minLatLon["lon"]:
                self.minLatLon["lon"] = lonlat["lon"]
            if lonlat["lat"] > self.maxLatLon["lat"]:
                self.maxLatLon["lat"] = lonlat["lat"]
            if lonlat["lon"] > self.maxLatLon["lon"]:
                self.maxLatLon["lon"] = lonlat["lon"]

    def add_osm_node(self, parent_elem, lonlat):

        if parent_elem is None:
            parent_elem = self.top

        self.check_minmax(lonlat)

        SubElement(parent_elem, "node", id=str(
            self.nodeIdx), visible="true", lat=str(lonlat["lat"]),
            lon=str(lonlat["lon"]))

        self.nodeIdx += 1

        return self.nodeIdx - 1

    def add_node_id(self, parent_elem, idx):

        SubElement(parent_elem, "nd", ref=str(idx))

    def add_highway(self, nodeIds, tags = {}):

        wayElement = SubElement(self.top, "way", id=str(self.wayIdx))
        self.wayIdx += 1
        self.highwayIdx += 1

        for n in nodeIds:
            self.add_node_id(wayElement, n)
        for k, v in tags.items():
            SubElement(wayElement, "tag", k=k, v=v)


    def add_rect_building(self, max_lonlat, min_lonlat, height):

        ne = {
            "lat": max_lonlat["lat"],
            "lon": max_lonlat["lon"]
        }

        nw = {
            "lat": max_lonlat["lat"],
            "lon": min_lonlat["lon"]
        }

        se = {
            "lat": min_lonlat["lat"],
            "lon": max_lonlat["lon"]
        }

        sw = {
            "lat": min_lonlat["lat"],
            "lon": min_lonlat["lon"]
        }

        first_node = self.add_osm_node(self.top, ne)

        nodeIds = [
            first_node,
            self.add_osm_node(self.top, nw),
            self.add_osm_node(self.top, sw),
            self.add_osm_node(self.top, se),
            first_node
        ]

        wayElement = SubElement(self.top, "way", id=str(self.wayIdx))
        self.wayIdx += 1
        self.buildingIdx += 1

        for n in nodeIds:
            self.add_node_id(wayElement, n)

        SubElement(wayElement, "tag", k="building", v="yes")
        SubElement(wayElement, "tag", k="height", v=str(height))

    def write(self, filename):

        self.bounds.set("minlat", str(self.minLatLon["lat"]))
        self.bounds.set("minlon", str(self.minLatLon["lon"]))
        self.bounds.set("maxlat", str(self.maxLatLon["lat"]))
        self.bounds.set("maxlon", str(self.maxLatLon["lon"]))

        with open(filename, 'wb') as f:
            self.tree.write(f, xml_declaration=True, encoding='utf-8')

        dom = xml.dom.minidom.parse(filename)
        
        with open(filename, 'w') as f:
            f.write(dom.toprettyxml())

def execute(extents: list[float], space: float, height: float, outputPath: str):

    builder = OSMBuilder()

    yidx = 0
    xidx = 0

    sw_corner = {}
    ne_corner = {}

    highway_nodes = []

    while True:
        if yidx * space * 2 > extents[3] - extents[1]:
            break

        xidx = 0
        while True:

            if xidx * space * 2 > extents[2] - extents[0]:
                break

            sw_corner = {
                "lat": extents[1] + space * yidx * 2,
                "lon": extents[2] + space * xidx * 2
            }

            ne_corner = {
                "lat": sw_corner["lat"] + space,
                "lon": sw_corner["lon"] + space
            }

            road_node = {
                "lat": ne_corner["lat"] + space * 0.5,
                "lon": ne_corner["lon"] + space * 0.5
            }

            highway_nodes.append(builder.add_osm_node(None, road_node))

            builder.add_rect_building(ne_corner, sw_corner, 30.0)

            xidx += 1

        yidx += 1

    for i in range(yidx):

        # print(highway_nodes[i:i+xidx-1])
        begin = i * xidx
        end = begin + xidx

        builder.add_highway(highway_nodes[begin:end], {
            "highway": "primry",
            "direction": "east-west"
        })

        north_south_nodes = []
        for j in range(xidx-1):
            index = j*xidx+i
            if index < len(highway_nodes):
                north_south_nodes.append(highway_nodes[j*xidx+i])

        builder.add_highway(north_south_nodes, {
            "highway": "primry",
            "direction": "north-south"
        })

    builder.write(outputPath)

    return [builder.buildingIdx, builder.highwayIdx]

if __name__ == "__main__":

    parser = argparse.ArgumentParser()

    parser.add_argument("output", type=str, default="test.osm")
    parser.add_argument("--extents", "-e", type=str,
                        default="0.0,0.0,0.001,0.001")
    parser.add_argument("--space", "-s", type=float, default=0.0002)
    parser.add_argument("--height", type=float, default=10.0)

    args = parser.parse_args()

    extents = [float(x) for x in args.extents.split(',')]

    execute(extents, args.space, args.height, args.output)