# -*- coding:utf-8 -*-

import argparse
from download import Download
from hparams import hparams as hps

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-url', required=True, help='radio server url')
    #parser.add_argument('-url',  default='http://mobilekazn.serverroom.us:6914', help='radio server url')
    #parser.add_argument('-url',  default='http://mobilekazn.serverroom.us:6914', help='radio server url')
    
    parser.add_argument('-path', required=True, help='radio path') 
    #parser.add_argument('-path', default='1300', help='radio path')
    #parser.add_argument('-path', default='1300', help='radio path')
    
    parser.add_argument('-zone', type=int, default=0, help='time zone (default: 0)')
    args = parser.parse_args()
    print(args.url, args.path, args.zone)
    downloader = Download(args.url, args.path, args.zone)
    downloader.run()
    
if __name__ == '__main__':
    main()