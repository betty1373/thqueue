# project thread queue for consumers/producers
Thread-save queue for multi-threaded app.
Use mutex and condition-variables 
May be used for small devices with std 11.
I tested it for multi-threaded app in PLC Devices, based on Linux with gcc 4.7.3 
(one consumer and many producers for logging)
[Документация](https://betty1373.github.io/thread_queue/)
