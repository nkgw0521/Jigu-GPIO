import argparse
from pathlib import Path
import subprocess
import tempfile
p=argparse.ArgumentParser();p.add_argument("--compiler",default="gcc");args=p.parse_args()
t=Path(__file__).resolve().parent;r=t.parents[1]
with tempfile.TemporaryDirectory(prefix="Jigu-PWM-common-") as tmp:
 for channel,pin in [(1,8),(2,14)]:
  exe=Path(tmp)/f"pwm_ch{channel}.exe"
  subprocess.run([args.compiler,"-std=c11","-Wall","-Wextra","-Werror",f"-DTEST_CHANNEL={channel}",f"-DTEST_PIN={pin}","-I"+str(t/"mocks"),"-I"+str(r/"App/Inc"),str(r/"App/Src/pwm_generator.c"),str(r/"App/Src/pwm_port_stm32_board_config.c"),str(t/"test_pwm.c"),"-o",str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
