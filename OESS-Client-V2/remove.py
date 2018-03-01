import urllib2
import urllib
import sys
import time
import string
import array
import numpy
global values
values = {}
file_name = str('Prov_serv_' + sys.argv[1] + '.log')
gh_url = 'https://al2s.net.internet2.edu/oess/services-kerb/provisioning.cgi'
if (sys.argv[1] == 'fail_over_circuit'):
        values = {'action' : 'fail_over_circuit', 'workgroup_id' : sys.argv[2], 'circuit_id' : sys.argv[3]}
elif (sys.argv[1] == 'reprovision_circuit'):
        values = {'action' : 'reprovision_circuit', 'workgroup_id' : sys.argv[2], 'circuit_id' : sys.argv[3]}
elif (sys.argv[1] == 'remove_circuit'):
        values = {'action' : 'remove_circuit', 'workgroup_id' : sys.argv[2], 'circuit_id' : sys.argv[3], 'remove_time' : -1}

data = urllib.urlencode(values)
req = urllib2.Request(gh_url, data)
#print data
password_manager = urllib2.HTTPPasswordMgrWithDefaultRealm()
password_manager.add_password(None, gh_url, 'uva4api', 'AL2Ssenyd2011!')

auth_manager = urllib2.HTTPBasicAuthHandler(password_manager)
opener = urllib2.build_opener(auth_manager)

urllib2.install_opener(opener)

handler = urllib2.urlopen(req)
f = open (file_name, 'w')
#for line in handler.read()
print handler.readline()
f.write(handler.read())
print handler.getcode()
#print handler.headers.getheader('content-type')
