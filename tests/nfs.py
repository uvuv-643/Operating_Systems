"""Service class for preparing context manager"""

import os
import subprocess
import requests
import json
import urllib.parse

from tests.constants import API_BASE, MOUNTPOINT, MODULE

class NfsObject:
    def __init__(self):
        r = requests.get("/".join([API_BASE, "token", "?json=123"]))
        assert r.json()['status'] == 0, "Can't get token from the server"
        self.token = r.json()['response']['token']
        subprocess.run(["mount", "-t", MODULE, self.token, MOUNTPOINT], stderr=subprocess.PIPE)
    
    def __enter__(self):
        return self
    
    def __exit__(self, type, value, traceback):
        try:
            subprocess.run(["umount", MOUNTPOINT], stderr=subprocess.PIPE)
        except:
            logging.warning("Can't umount filesystem")
    
    def clear(self, dir_inode = 0):
        r = requests.get("/".join([API_BASE, "list?json=1&token=" + self.token]) + "&inode=" + str(dir_inode))
        assert r.json()['status'] == 0, "Can't get list from the server"
        files = r.json()['response']
        entries = files['entries']
        for entry in entries:
            entry_type = entry['entry_type']
            ino = entry['ino']
            name = entry['name']
            if (entry_type == 4):
                clear(self, ino)
                requests.get("/".join([API_BASE, "rmdir?json=1&token=" + self.token]) + "&parent=" + str(dir_inode) + "&name=" + name)
            else:
                r = requests.get("/".join([API_BASE, "unlink?json=1&token=" + self.token]) + "&parent=" + str(dir_inode) + "&name=" + name)
    
    def create(self, path, inode_type):
        assert inode_type == "file" or inode_type == "directory", "Expected file or directory"
        names = path.split('/')
        cur_inode = 0
        for name in names[:-1]:
            if name == '':
                continue
            r = requests.get("/".join([API_BASE, "lookup?json=1&token=" + self.token]) + "&parent=" + str(cur_inode) + "&name=" + name)
            cur_inode = r.json()['response']['ino']
        r = requests.get("/".join([API_BASE, "create?json=1&token=" + self.token]) + "&parent=" + str(cur_inode) + "&name=" + names[-1] + "&type=" + inode_type)

    def list(self, path):
        names = path.split('/')
        cur_inode = 0
        for name in names[:-1]:
            if name == '':
                continue
            r = requests.get("/".join([API_BASE, "lookup?json=1&token=" + self.token]) + "&parent=" + str(cur_inode) + "&name=" + name)
            cur_inode = r.json()['response']['ino']
        r = requests.get("/".join([API_BASE, "list?json=1&token=" + self.token]) + "&inode=" + str(cur_inode));
        ret = []
        entries = r.json()['response']['entries']
        for entry in entries:
            ret.append(entry['name'])
        return ret

    def write(self, path, content):
        names = path.split("/")
        cur_inode = 0
        for name in names:
            if name == '':
                continue
            r = requests.get("/".join([API_BASE, "lookup?json=1&token=" + self.token]) + "&parent=" + str(cur_inode) + "&name=" + name)
            cur_inode = r.json()['response']['ino']
        requests.get("/".join([API_BASE, "write?json=1&token=" + self.token]) + "&inode=" + str(cur_inode) + "&content=" + urllib.parse.quote_plus(content))

    def read(self, path):
        names = path.split("/")
        cur_inode = 0
        for name in names:
             if name == '':
                 continue
             r = requests.get("/".join([API_BASE, "lookup?json=1&token=" + self.token]) + "&parent=" + str(cur_inode) + "&name=" + name)
             cur_inode = r.json()['response']['ino']
        r = requests.get("/".join([API_BASE, "read?json=1&token=" + self.token]) + "&inode=" + str(cur_inode))
        return r.json()['response']['content']