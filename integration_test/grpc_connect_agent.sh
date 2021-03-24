#!/bin/bash
TOKEN=$(awk '{ print $2 }' token)
spire-agent run -joinToken $TOKEN -config /opt/spire/conf/agent/agent.conf &
