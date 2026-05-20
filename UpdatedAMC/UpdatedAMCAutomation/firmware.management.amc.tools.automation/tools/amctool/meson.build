project('amctool', 'c', version: '1.0.0')

# Dependencies

dbus_dep = dependency('dbus-1')
cjson_dep = dependency('libcjson')

executable('amctool', ['pldm/pldm.c', 
            'pldm/utils.c',
            'pldm/update_agent.c',
            'pldm/fwpkg.c',
            'pldm/pldm_oem.c',
            'pldm/pldm_fru.c',
            'pldm/pldm_mc.c',
            'pldm/numeric_se.c',
            'pldm/state_se.c',
            'pldm/config.c',
            'mctp/mctp.c', 
            'mctp/smbus.c', 
            'mctp/mctp_control.c',
            'libpldm/base.c', 
            'libpldm/utils.c', 
            'libpldm/firmware_update.c',
            'libpldm/fru.c',
            'libpldm/platform.c',
            'libpldm/pdr.c'],
            dependencies: [dbus_dep, cjson_dep])
