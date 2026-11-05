function reg = registry_load(path)
  % Load a registry JSON file into a struct (jsondecode).
  % Homogeneous object arrays -> struct arrays; heterogeneous -> cell.
  % Normalize entries to a column cell of structs for uniform indexing.
  txt = fileread(path);
  reg = jsondecode(txt);
  if isstruct(reg.entries)
    c = cell(numel(reg.entries), 1);
    for i = 1:numel(reg.entries)
      c{i} = reg.entries(i);
    end
    reg.entries = c;
  end
end
