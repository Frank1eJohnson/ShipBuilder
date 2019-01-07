#!/usr/bin/env python
# -*- coding: utf-8 -*-

#-------------------------------------------------------------------------------
# Upload the game for distribution - make sure to configure Build.json
# Usage : UploadItch.py <absolute-output-dir>
# 
# Gwennaël Arbona 2021
#-------------------------------------------------------------------------------

import os
import json
import subprocess


#-------------------------------------------------------------------------------
# Read config files for data
#-------------------------------------------------------------------------------

configDir = '../Config/'
projectConfigFile = open(os.path.join(configDir, 'Build.json'))
projectConfig = json.load(projectConfigFile)

# Get optional build settings
outputDir =                  str(projectConfig.get('outputDir'))

# Get Itch settings
itchConfig =                 projectConfig["itch"]
itchProject =                str(itchConfig["project"])
itchBranches =               itchConfig["branches"]
itchDirectories =            itchConfig["directories"]


#-------------------------------------------------------------------------------
# Process arguments for sanity
#-------------------------------------------------------------------------------
	
# Get output directory
if outputDir == 'None':
	if len(sys.argv) == 2:
		outputDir = sys.argv[1]
	else:
		sys.exit('Output directory was neither set in Build.json nor passed as command line')
		
# Get user name
if 'ITCH_USER' in os.environ:
	itchUser = os.environ['ITCH_USER']
else:
	sys.exit('itch.io user was not provided in the ITCH_USER environment variable')


#-------------------------------------------------------------------------------
# Upload all platforms to itch.io
#-------------------------------------------------------------------------------

itchBranchIndex = 0
for branch in itchBranches:

	# Upload
	subprocess.check_call([
		'butler',
		'push',
		os.path.join(outputDir, itchDirectories[itchBranchIndex]),
		itchUser + "/" + itchProject + ":" + branch
	])
	
	itchBranchIndex += 1
