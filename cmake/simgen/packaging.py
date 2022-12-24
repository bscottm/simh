import os


class SIMHPackaging:
    def __init__(self, family, install_flag = True) -> None:
        self.family = family
        self.processed = False
        self.install_flag = install_flag

    def was_processed(self) -> bool:
        return self.processed == True
    
    def encountered(self) -> None:
        self.processed = True

class PkgFamily:
    def __init__(self, component_name, display_name, description) -> None:
        self.component_name = component_name
        self.display_name   = display_name
        self.description    = description

    def write_component_info(self, stream, indent) -> None:
        indent0 = ' ' * indent
        indent4 = ' ' * (indent + 4)
        stream.write(indent0 + "cpack_add_component(" + self.component_name + "\n")
        stream.write(indent4 + "DISPLAY_NAME \"" + self.display_name + "\"\n")
        stream.write(indent4 + "DESCRIPTION \"" + self.description + "\"\n")
        stream.write(indent0 + ")\n")


def write_packaging(toplevel_dir) -> None:
    families = set([sim.family for sim in package_info.values()])
    pkging_file = os.path.join(toplevel_dir, 'cmake', 'simh-packaging.cmake')
    print("==== writing {0}".format(pkging_file))
    with open(pkging_file, "w") as stream:
        for family in families:
            family.write_component_info(stream, 0)

default_family = PkgFamily("simh_suite", "The SIMH simulator suite",
    """The SIMH simulator collection of historical processors and computing systems"""
)
    
att3b2_family = PkgFamily("att3b2_family", "ATT&T 3b2 collection",
    """The AT&T 3b2 simulator family"""
)

vax_family = PkgFamily("vax_family", "DEC VAX simulator collection",
    """The Digital Equipment Corporation VAX (plural: VAXen) simulator family."""
)

pdp10_family = PkgFamily("pdp10_family", "DEC PDP-10 collection",
    """DEC PDP-10 architecture simulators and variants."""
)

pdp11_family = PkgFamily("pdp11_family", "DEC PDP-11 collection",
   """DEC PDP-11 and PDP-11-derived architecture simulators."""
)

experimental_family = PkgFamily("experimental", "Experimental (work-in-progress) simulators",
    """Experimental or work-in-progress simulators not in the SIMH mainline simulator suite."""
)

package_info = {
    "3b2": SIMHPackaging(att3b2_family),
    "3b2-700": SIMHPackaging(att3b2_family),
    "altair": SIMHPackaging(default_family),
    "altairz80": SIMHPackaging(default_family),
    "b5500": SIMHPackaging(default_family),
    "besm6": SIMHPackaging(default_family),
    "cdc1700": SIMHPackaging(default_family),
    "eclipse": SIMHPackaging(default_family),
    "gri": SIMHPackaging(default_family),
    "h316": SIMHPackaging(default_family),
    "hp2100": SIMHPackaging(default_family),
    "hp3000": SIMHPackaging(default_family),
    "i1401": SIMHPackaging(default_family),
    "i1620": SIMHPackaging(default_family),
    "i650": SIMHPackaging(default_family),
    "i701": SIMHPackaging(default_family),
    "i7010": SIMHPackaging(default_family),
    "i704": SIMHPackaging(default_family),
    "i7070": SIMHPackaging(default_family),
    "i7080": SIMHPackaging(default_family),
    "i7090": SIMHPackaging(default_family),
    "i7094": SIMHPackaging(default_family),
    "ibm1130": SIMHPackaging(default_family),
    "id16": SIMHPackaging(default_family),
    "id32": SIMHPackaging(default_family),
    "imlac": SIMHPackaging(default_family),
    "infoserver100": SIMHPackaging(vax_family),
    "infoserver1000": SIMHPackaging(vax_family),
    "infoserver150vxt": SIMHPackaging(vax_family),
    "intel-mds": SIMHPackaging(default_family),
    "lgp": SIMHPackaging(default_family),
    "microvax1": SIMHPackaging(vax_family),
    "microvax2": SIMHPackaging(vax_family),
    "microvax2000": SIMHPackaging(vax_family),
    "microvax3100": SIMHPackaging(vax_family),
    "microvax3100e": SIMHPackaging(vax_family),
    "microvax3100m80": SIMHPackaging(vax_family),
    "nova": SIMHPackaging(default_family),
    "pdp1": SIMHPackaging(default_family),
    ## Don't install pdp10 per Rob Cromwell
    "pdp10": SIMHPackaging(pdp10_family, False),
    "pdp10-ka": SIMHPackaging(pdp10_family),
    "pdp10-ki": SIMHPackaging(pdp10_family),
    "pdp10-kl": SIMHPackaging(pdp10_family),
    "pdp10-ks": SIMHPackaging(pdp10_family),
    "pdp11": SIMHPackaging(pdp11_family),
    "pdp15": SIMHPackaging(default_family),
    "pdp4": SIMHPackaging(default_family),
    "pdp6": SIMHPackaging(default_family),
    "pdp7": SIMHPackaging(default_family),
    "pdp8": SIMHPackaging(default_family),
    "pdp9": SIMHPackaging(default_family),
    "rtvax1000": SIMHPackaging(vax_family),
    "s3": SIMHPackaging(default_family),
    "scelbi": SIMHPackaging(default_family),
    "sds": SIMHPackaging(default_family),
    "sel32": SIMHPackaging(default_family),
    "sigma": SIMHPackaging(default_family),
    "ssem": SIMHPackaging(default_family),
    "swtp6800mp-a": SIMHPackaging(default_family),
    "swtp6800mp-a2": SIMHPackaging(default_family),
    "tt2500": SIMHPackaging(default_family),
    "tx-0": SIMHPackaging(default_family),
    "uc15": SIMHPackaging(pdp11_family),
    "vax": SIMHPackaging(vax_family),
    "vax730": SIMHPackaging(vax_family),
    "vax750": SIMHPackaging(vax_family),
    "vax780": SIMHPackaging(vax_family),
    "vax8200": SIMHPackaging(vax_family),
    "vax8600": SIMHPackaging(vax_family),
    "vaxstation3100m30": SIMHPackaging(vax_family),
    "vaxstation3100m38": SIMHPackaging(vax_family),
    "vaxstation3100m76": SIMHPackaging(vax_family),
    "vaxstation4000m60": SIMHPackaging(vax_family),
    "vaxstation4000vlc": SIMHPackaging(vax_family),

    ## Experimental simulators:
    "alpha": SIMHPackaging(experimental_family),
    "pdq3": SIMHPackaging(experimental_family),
    "sage": SIMHPackaging(experimental_family)
}