#!/bin/sh

cd ../root
sys161 kernel "km1; km2; km3 200000; p /bin/true; p /testbin/huge; p /testbin/triplehuge; p /testbin/parallelvm; q"

sys161 kernel "km1; km2; km3 20000; p /bin/true; p /testbin/huge; p /testbin/triplehuge; p /testbin/parallelvm; p /bin/true; p /testbin/huge; q"

sys161 kernel "km1; km2; km3 20000; p /bin/true; p /testbin/huge; p /testbin/triplehuge; p /testbin/parallelvm; p /testbin/triplehuge; p /testbin/triplehuge; p /testbin/triplehuge; p /testbin/triplehuge; p /testbin/huge; p /testbin/parallelvm; p /testbin/huge; p /testbin/parallelvm; p /bin/true; q"

sys161 kernel "km1; km2; km3 20000; p /bin/true; p /testbin/huge; p /testbin/triplehuge; p /testbin/parallelvm; p /testbin/triplehuge; p /testbin/triplehuge; p /testbin/triplehuge; p /testbin/triplehuge; p /testbin/triplehuge; p /testbin/triplehuge; p /testbin/triplehuge; p /testbin/triplehuge; p /testbin/triplehuge; p /testbin/triplehuge; p /testbin/triplehuge; p /testbin/triplehuge; p /testbin/triplehuge; p /testbin/triplehuge; p /testbin/triplehuge; p /bin/true; p /testbin/huge; q"
