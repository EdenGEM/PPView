#!/bin/usr/env python 
# -*- coding: utf-8 -*-

import sys
import re
import json
import time

NeedErr = True
NeedLog = True
NeedDbg = True
NeedWarning = True

def GetStamp():
	return time.strftime("%Y%m%d_%H:%M:%S", time.localtime(time.time()))	

def PrintErr(line):
	if not NeedErr:
		return
	head = '[ERR][%s]' % (GetStamp())
	print >> sys.stderr, head + line + '\n',;

def PrintLog(line):
	if not NeedLog:
		return
	head = '[LOG][%s]' % (GetStamp())
	print >> sys.stderr, head + line + '\n',;

def PrintDbg(line):
	if not NeedDbg:
		return
	head = '[DBG][%s]' % (GetStamp())
	print >> sys.stderr, head + line + '\n',;

def PrintWarning(line):
	if not NeedWarning:
		return
	head = '[WARNING][%s]' % (GetStamp())
	print >> sys.stderr, head + line + '\n',;

def Init():
	global NeedLog, NeedDbg, NeedErr
	NeedDbg = False
#	NeedErr = False
#	NeedErr = False
#	NeedErr = False
	pass

def main():
	Init()
	PrintErr("test Err");
	PrintDbg("test Dbg");
	PrintLog("test Log");
	PrintWarning("test Warning");

if __name__ == "__main__":
	main()
