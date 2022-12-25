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

altairz80_family = PkgFamily("altairz80_family", "Altair Z80 simulator",
    """The Altair Z80 simulator with M68000 support."""
)

b5500_family = PkgFamily("b5500_family", "Burroughs 5500",
    """The Burroughs 5500 system simulator""")

cdc1700_family = PkgFamily("cdc1700_family", "CDC 1700",
    """The Control Data Corporation's CDC-1700 system simulator"""
)

dgnova_family = PkgFamily("dgnova_family", "DG Nova and Eclipse",
    """Data General NOVA and Eclipse system simulators"""
)

grisys_family = PkgFamily("grisys_family", "GRI Systems GRI-909",
    """GRI Systems GRI-909 system simulator"""
)

honeywell_family = PkgFamily("honeywell_family", "Honeywell H316",
    """Honeywell H-316 system simulator"""
)

hp_family = PkgFamily("hp_family", "HP 2100, 3000",
    """Hewlett-Packard H2100 and H3000 simulators""")

ibm_family = PkgFamily("ibm_family", "IBM",
    """IBM system simulators: i650"""
)

imlac_family = PkgFamily("imlac_family", "IMLAC",
    """IMLAC system simulators"""
)

intel_family = PkgFamily("intel_family", "Intel",
    """Intel system simulators"""
)

interdata_family = PkgFamily("interdata_family", "Interdata",
    """Interdata systems simulators: id16, id32"""
)

lgp_family = PkgFamily("lgp_family", "LGP",
    """Librascope systems simulators"""
)

decpdp_family = PkgFamily("decpdp_family", "DEC PDP family",
    """Digital Equipment Corporation PDP system simulators"""
)

sds_family = PkgFamily("sds_family", "SDS simulators",
    """Scientific Data Systems (SDS) system simulators"""
)

gould_family = PkgFamily("gould_family", "Gould simulators",
    """Gould Systems simulators"""
)

swtp_family = PkgFamily("swtp_family", "SWTP simulators",
    """Southwest Technical Products (SWTP) system simulators"""
)


package_info = {
    "3b2": SIMHPackaging(att3b2_family),
    "3b2-700": SIMHPackaging(att3b2_family),
    "altair": SIMHPackaging(default_family),
    "altairz80": SIMHPackaging(altairz80_family),
    "b5500": SIMHPackaging(b5500_family),
    "besm6": SIMHPackaging(default_family),
    "cdc1700": SIMHPackaging(cdc1700_family),
    "eclipse": SIMHPackaging(dgnova_family),
    "gri": SIMHPackaging(grisys_family),
    "h316": SIMHPackaging(honeywell_family),
    "hp2100": SIMHPackaging(hp_family),
    "hp3000": SIMHPackaging(hp_family),
    "i1401": SIMHPackaging(ibm_family),
    "i1620": SIMHPackaging(ibm_family),
    "i650": SIMHPackaging(ibm_family),
    "i701": SIMHPackaging(ibm_family),
    "i7010": SIMHPackaging(ibm_family),
    "i704": SIMHPackaging(ibm_family),
    "i7070": SIMHPackaging(ibm_family),
    "i7080": SIMHPackaging(ibm_family),
    "i7090": SIMHPackaging(ibm_family),
    "i7094": SIMHPackaging(ibm_family),
    "ibm1130": SIMHPackaging(ibm_family),
    "id16": SIMHPackaging(interdata_family),
    "id32": SIMHPackaging(interdata_family),
    "imlac": SIMHPackaging(imlac_family),
    "infoserver100": SIMHPackaging(vax_family),
    "infoserver1000": SIMHPackaging(vax_family),
    "infoserver150vxt": SIMHPackaging(vax_family),
    "intel-mds": SIMHPackaging(intel_family),
    "lgp": SIMHPackaging(lgp_family),
    "microvax1": SIMHPackaging(vax_family),
    "microvax2": SIMHPackaging(vax_family),
    "microvax2000": SIMHPackaging(vax_family),
    "microvax3100": SIMHPackaging(vax_family),
    "microvax3100e": SIMHPackaging(vax_family),
    "microvax3100m80": SIMHPackaging(vax_family),
    "nova": SIMHPackaging(dgnova_family),
    "pdp1": SIMHPackaging(decpdp_family),
    ## Don't install pdp10 per Rob Cromwell
    "pdp10": SIMHPackaging(pdp10_family, install_flag=False),
    "pdp10-ka": SIMHPackaging(pdp10_family),
    "pdp10-ki": SIMHPackaging(pdp10_family),
    "pdp10-kl": SIMHPackaging(pdp10_family),
    "pdp10-ks": SIMHPackaging(pdp10_family),
    "pdp11": SIMHPackaging(pdp11_family),
    "pdp15": SIMHPackaging(decpdp_family),
    "pdp4": SIMHPackaging(decpdp_family),
    "pdp6": SIMHPackaging(decpdp_family),
    "pdp7": SIMHPackaging(decpdp_family),
    "pdp8": SIMHPackaging(default_family),
    "pdp9": SIMHPackaging(decpdp_family),
    "rtvax1000": SIMHPackaging(vax_family),
    "s3": SIMHPackaging(ibm_family),
    "scelbi": SIMHPackaging(intel_family),
    "sds": SIMHPackaging(sds_family),
    "sel32": SIMHPackaging(gould_family),
    "sigma": SIMHPackaging(sds_family),
    "ssem": SIMHPackaging(default_family),
    "swtp6800mp-a": SIMHPackaging(swtp_family),
    "swtp6800mp-a2": SIMHPackaging(swtp_family),
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