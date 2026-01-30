1) config.h = “রোবটের রুলবুক”

এখানে লেখা থাকে রোবটের সব নিয়ম-কানুন:

কোন পিন কোথায় লাগানো

PWM কত kHz

লাইন কালো না সাদা (LINE_IS_BLACK)

PID গেইন (Kp, Ki, Kd)

বেস স্পিড, ম্যাক্স স্পিড

কতটা সেন্সর ফিল্টার হবে

লাইন হারালে কী করবে

মানে: সব সেটিংস এক জায়গায়, যাতে ডিবাগ/টিউন করা একদম সহজ।

2) MotorTB6612 = “মাংসপেশি (Muscles)”

এই ক্লাসটা মোটর চালায়। কিন্তু সে “হঠাৎ ঝাঁপ” দিয়ে স্পিড বদলায় না—স্মার্টলি বদলায়।

Motor চরিত্র কী করে?

begin() → মোটরের পিন সেট করে, PWM চালু করে

setTarget(left,right) → তাকে বলে “লক্ষ্য স্পিড কত”

update() → ধীরে ধীরে সেই লক্ষ্য স্পিডে পৌঁছায় (slew limit)

stop() → থামায়

কেন update() এ slewing আছে?

যদি এক লুপে 0 থেকে 200 PWM করে দাও:

কারেন্ট spike হয়

ভোল্টেজ ডিপ হয়

ESP32 reset/heat হতে পারে

রোবট jerk করে

তাই সে প্রতি লুপে একটু একটু করে এগোয়: SLEW_STEP_PER_LOOP

এটা একদম রেস কারের মতো—smooth acceleration।

3) SensorMux12 = “চোখ (Eyes)”

তোমার কাছে 12টা সেন্সর কিন্তু 12টা ADC পিন নেই। তাই mux দিয়ে পড়তে হয়।

Sensor চোখ কীভাবে দেখে?

সে আগে SEL_A আর SEL_B দিয়ে বলে:

“এখন 00 গ্রুপ দেখবো”

“এখন 01 গ্রুপ দেখবো”

“এখন 10 গ্রুপ দেখবো”

“এখন 11 গ্রুপ দেখবো”

আর IN_A/IN_B/IN_C/IN_D দিয়ে বিভিন্ন সেন্সরের ভ্যালু পড়ে raw_[12] এ রাখে।

তারপর সে “বোঝে” (Calibration + Normalize)

রোবটের চোখ সবসময় একই আলো/ফ্লোর পায় না। তাই সে আগে “ট্রেনিং” নেয়:

resetCalibration() → min/max রিসেট

calibrateStep() → প্রতিবার raw পড়ে min/max আপডেট

finishCalibration() → রেঞ্জ খুব ছোট হলে ঠিক করে

তারপর updateFiltered():

raw (0..4095) কে normalize করে (0..1000)

তারপর ফিল্টার করে (EMA low pass) → noise কমায়

মানে চোখ কাঁচা ডাটা নয়, পরিষ্কার ডাটা দেয়।

Line position বের করা (চোখের ম্যাথ)

linePosition(pos,sum):

12 সেন্সরের “লাইন শক্তি” যোগ করে sum

প্রতিটা সেন্সরের index × শক্তি যোগ করে weighted sum

শেষে pos বের করে (0..11000)

pos=5500 মানে একদম মাঝখানে।

4) LineController = “মস্তিষ্ক (Brain)”

এই ফাইলটা PID করে।

update(error, dt) → correction দেয়

এখানে:

error = pos - 5500

dt = দুই লুপের সময় পার্থক্য

Brain বলে:

“লাইন ডানে চলে গেছে? correction দিয়ে ডানে ঘোরাও”

“লাইন বামে? বামে ঘোরাও”

“কাঁপছে? derivative দিয়ে ড্যাম্প করো”

5) main.cpp = “কোচ/ম্যানেজার (Race Engineer)”

সবচেয়ে মজার অংশ—এখানেই পুরো গল্পটা চলে।

সে ৪টা মোড চালায়:

MODE_CALIB → ক্যালিব্রেশন

MODE_RUN → লাইনে দৌড়

MODE_SENSORS → শুধু সেন্সর স্ট্রিম (ডিবাগ)

MODE_STOP → থামা

গল্প শুরু: রোবট ঘুম থেকে উঠল (setup())

Serial চালু

মোটর চালু (motors.begin())

সেন্সর চালু (sensors.begin())

PID চালু (ctrl.begin())

হেল্প প্রিন্ট

তারপর সরাসরি “ট্রেনিং” → startCalib()

Scene-1: Calibration Dance (রোবটের ট্রেনিং)

MODE_CALIB এ রোবট ছোট ছোট সময় ধরে ডানে-বামে স্পিন করে।

কেন স্পিন?
যাতে সেন্সররা:

লাইন দেখুক

ফ্লোর দেখুক

দুইটার min/max শিখুক

এই সময়:

sensors.readRaw()

sensors.calibrateStep()

ক্যালিব্রেশন শেষ হলে:

মোটর থামে

calibration finalize হয়

তারপর সে STOP মোডে যায়

তুমি চাইলে r লিখে RUN শুরু করবে

Scene-2: Race Start (RUN মোড)

RUN মোডে প্রতি লুপে “চ্যাম্পিয়ন রুটিন”:

চোখ দেখে
sensors.readRaw()
sensors.updateFiltered()

লাইন আছে কি না দেখে
sensors.linePosition(pos,sum)

যদি লাইন থাকে:

error বের করে

ctrl.update(error,dt) থেকে correction নেয়

বেস স্পিড turns এ কমায় (speed scheduling)

তারপর মোটরের target দেয়
motors.setTarget(left,right)

smooth ভাবে apply করে
motors.update()

speed scheduling কেন?

লাইন মাঝখানে থাকলে রোবট দ্রুত যায়।
error বড় হলে রোবট গতি কমায় যাতে টার্ন ঠিক হয়।

এটাই চ্যাম্পিয়নদের ট্রিক: সোজা পথে fast, টার্নে safe।

Scene-3: Line Lost Recovery (লাইন হারিয়ে গেলে)

যদি sum < LINE_SUM_THRESHOLD, চোখ বলে: “আমি লাইন পাচ্ছি না!”

তখন কোচ একটা plan চালায়:

lostCount++

lastError যে দিক দেখায়, সেই দিকে search turn করে

বেশি সময় হারালে শেষ পর্যন্ত stop

মানে রোবট উল্টা-পাল্টা দৌড়ায় না, বুদ্ধি করে খুঁজে।

Debugging story: Serial কমান্ড হলো “কোচের রিমোট”

তুমি সিরিয়ালে লিখলে কোচ সেটা শুনে:

c → “আবার ট্রেনিং কর”

r → “রেস শুরু”

s → “চোখ কী দেখছে দেখাও”

x → “থাম”

p → PID print

k kp ki kd → লাইভ PID বদলাও

এটাই modular কোডের আসল শক্তি—রান টাইম টিউনিং।

যদি কিছু উল্টো হয় (চ্যাম্পিয়ন ফিক্স)
1) রোবট লাইন থেকে দূরে পালায়

LINE_IS_BLACK উল্টো সেট করা।

কালো লাইন হলে true

সাদা লাইন হলে false

2) ডানে যেতে গেলে বামে যায়

মোটর polarity উল্টো।
MOTOR_SIGN_L বা MOTOR_SIGN_R -1 করে দাও।

3) কাঁপে/oscillate

Kd একটু কমাও (0.16 → 0.12)

বা Kp একটু কমাও

4) টার্ন নিতে পারে না

Kp একটু বাড়াও (0.055 → 0.065)

BASE_SPEED একটু কমাও

TURN_SLOW বাড়াও (0.45 → 0.55)

শেষ কথা (একটা লাইনেই)

এই কোডে:

চোখ দেখে (SensorMux12),

মস্তিষ্ক হিসাব করে (PID Controller),

মাংসপেশি কাজ করে (MotorTB6612),

আর কোচ সবকিছু কন্ট্রোল করে (main.cpp + serial commands)।
