tiva-ussh
=========

Tiva Connected Launchpad micro SSH implementation.

Based on lwip included in TI's Tivaware.

Uses ADC to generate random bits for device's key. ADC is still available for other uses. The internal temperature sensor is used for random bits because it is non-trivial to control its entropy.
