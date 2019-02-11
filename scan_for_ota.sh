#!/bin/bash
watch -n1 "avahi-browse _arduino._tcp --resolve --parsable --terminate"
