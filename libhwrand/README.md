# Cryptographic Hardware Random Rumbers

The TM4C1294NCPDT does not include AES hardware due to US export restrictions. However, it has an internal temperature sensor that provides a cryptographically secure hardware random number generator.

tiva-ussh uses a random number library (libhwrand) that reads the temperature sensor to get random bits. The bits are evaluated to resist tampering. Then the bits are fed into a pseudo-random generator.

# Side Effects

**Initialization**: To keep libhwrand lightweight, it does not configure the ADC, but relies on the calling application to do that. It means you must set up the ADC clock, triggers, sequencers, and interrupts or polling. This allows your application to decide when libhwrand gets priority to sample the temp sensor.

Refer to the libhwrand sample app for more information.

**Temp reading**: libhwrand provides a cleaned-up 12-bit raw temperature value for free via `get_ADC_temperature()`. libhwrand wants the random bits, so it must subtract the actual temperature from each reading. `get_ADC_temperature()` returns -1 if the pool has not been initialized.

# Sampling

Most applications read the ADC continually by design. The temp sensor must be included at low priority in the ADC sequence the application uses. The libhwrand sample app demonstrates this idea.

**Exhaustion**: libhwrand keeps a pool of random bits for when needed. If the application needs more random bits than have been generated, libhwrand returns -1 from `get_hwrand()`. The application must then feed more ADC samples to libhwrand before `get_hwrand()` will succeed.

The temp sensor is oversampled to get a clean average. The least significant bits of each reading are fed into the pool. libhwrand needs 8 samples to initialize the pool, and 8 more per byte returned by `get_hwrand()`. `get_ADC_temperature()` averages the last 512 (TBD: what effect does CPU frequence have?) samples.

# Expanding on get_hwrand()

The pseudo-random generator can be used to get more random numbers as well. This will not reduce the pool for `get_hwrand()`. There is a trade-off though: random numbers obtained from `get_swrand()` are not as secure. `get_swrand()` checks to make sure the pool was correctly seeded, but if random numbers are exposed to an attacker (typically through a Diffie-Helman key exchange), the internal state of the pseudo-random generator can be backed out with enough samples.

`get_swrand()` will be re-seeded as more samples become available but should be used with caution.

# Size

libhwrand uses TBD flash and TBD ram.

# Future improvements

If the application has a chance to save data to EEPROM when powering down (risky: the whole EEPROM can be corrupted if power drops too much), it can call `libhwrand_add_bits()` directly on the next startup to help seed the entropy pool. Data for saving can be from `get_swrand()` since it is not exposed to an attacker.
