#!/usr/bin/env python3

from xml.etree.ElementTree import Element, ElementTree, SubElement, Comment, tostring


class OSMBuilder:

  def __init__(self):

    self.nodeIdx = 0
    self.wayIdx = 0

    self.top = Element("osm")
    self.top.set('generator', 'osm2assimp test generator')
    self.top.set('version', '0.6')

    self.tree = ElementTree(self.top)

  def add_osm_node(self, parent_elem, lonlat):

    nodeElement = SubElement(parent_elem, "node", id=str(self.nodeIdx), visible="true", version="2", changeset="1", lat=str(lonlat["lat"]), lon=str(lonlat["lon"]))

    self.nodeIdx += 1;

    return self.nodeIdx - 1

  def add_node_id(self, parent_elem, idx):

    nodeIdElem = SubElement(parent_elem, "nd", ref=str(idx))

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

    nodeIds = [
      self.add_osm_node(self.top, ne),
      self.add_osm_node(self.top, nw),
      self.add_osm_node(self.top, se),
      self.add_osm_node(self.top, sw)
    ]

    wayElement = SubElement(self.top, "way", id=str(self.wayIdx))
    self.wayIdx += 1

    for n in nodeIds:
      self.add_node_id(wayElement, n)

    SubElement(wayElement, "tag", k="building", v="yes")
    SubElement(wayElement, "tag", k="height", v=str(height))
    
  def write(self, filename):

    with open(filename, 'wb') as f:
        self.tree.write(f, xml_declaration=True, encoding='utf-8')


builder = OSMBuilder()

sw = {
  "lat": 0.0,
  "lon": 0.0
}

ne = {
  "lat": 0.0001,
  "lon": 0.0001
}

builder.add_rect_building(ne, sw, 30.0)

builder.write("test.osm")