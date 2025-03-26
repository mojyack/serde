add_project_arguments('-Wno-missing-field-initializers', language: 'cpp')

subdir('serde')

executable('json',
  files(
    'json.cpp',
  ) + serde_json_files,
)

executable('xml',
  files(
    'xml.cpp',
  ) + serde_xml_files,
)

executable('bin',
  files(
    'bin.cpp',
  )
)
