"""Run on the production firmware, with PWM wired to its external counter input."""
import argparse,json,time
from pathlib import Path
import serial
p=argparse.ArgumentParser();p.add_argument('--port',required=True);p.add_argument('--output',required=True);p.add_argument('--extended',action='store_true');a=p.parse_args()
rows=[]
with serial.Serial(a.port,115200,timeout=.05,write_timeout=2) as s:
 def cmd(c):
  s.reset_input_buffer();s.write((c+'\n').encode());s.flush()
  deadline=time.monotonic()+2; buf=b''
  while time.monotonic()<deadline:
   buf+=s.read(256)
   lines=[x.strip() for x in buf.decode(errors='replace').splitlines() if x.strip() and x.strip()!=c]
   if '?' not in c or lines:return lines[-1] if lines else ''
  raise RuntimeError((c,buf))
 def run(hz,width,pol,n,seed=0,reset=True):
  cmd('PWM:STart 0');cmd('PWM:Frequency '+str(hz));cmd('PWM:Width '+str(width));cmd('PWM:POLarity '+str(pol));cmd('PWM:Numbers '+str(n));cmd('PWM:SYNC 0')
  if reset:cmd('PWM:Counter '+str(seed))
  else:seed=int(cmd('PWM:Counter?'))
  cmd('PWM:STart 1');deadline=time.monotonic()+n/hz+3
  while cmd('PWM:STart?')!='0':
   if time.monotonic()>deadline:raise RuntimeError('stop timeout')
  actual=int(cmd('PWM:Counter?'));time.sleep(.02);stable=int(cmd('PWM:Counter?'));err=cmd('SYSTem:ERRor?')
  expected=(seed+n)&(0xffffffff if a.extended else 0xffff)
  row=dict(Hz=hz,WidthUs=width,Polarity=pol,Pulses=n,Seed=seed,Actual=actual,Expected=expected,Stable=stable,Error=err,Pass=actual==expected and stable==actual and err.startswith('0,'))
  rows.append(row)
  Path(a.output).write_text(json.dumps(dict(Port=a.port,Results=rows),indent=2)+'\n')
  if not row['Pass']:raise AssertionError(row)
 try:
  for pol in [0,1]:
   for hz,widths in [(50,[1,10,100,500]),(1000,[1,10,100,500]),(10000,[1,10])]:
    for width in widths:
     for n in [1,2,10]:
      for repeat in range(3):run(hz,width,pol,n)
    print(a.port,'polarity',pol,'Hz',hz,'passed',len(rows),flush=True)
   for repeat in range(10):run(1000,10,pol,1,reset=False)
   if a.extended:
    for seed,n in [(65530,20),(131060,20),(0xfffffff8,20),(0,70000)]:run(10000,10,pol,n,seed)
 finally:
  cmd('PWM:STart 0');cmd('PWM:Frequency 50');cmd('PWM:Width '+('100' if a.extended else '1'));cmd('PWM:POLarity 1');cmd('PWM:Numbers 0');cmd('PWM:SYNC 0');cmd('PWM:Counter 0')
print('PASS',a.port,len(rows))
