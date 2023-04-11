import os
import json
import zipfile

import os
import zipfile

def get_directory_structure(path):
    directory = {'name': os.path.basename(path), 'path': path}
    if os.path.isdir(path):
        directory['children'] = [get_directory_structure(os.path.join(path, child)) for child in os.listdir(path)]
        directory['size'] = sum([child.get('size', 0) for child in directory['children']]) + sum([os.path.getsize(os.path.join(path, child)) for child in os.listdir(path) if not os.path.isdir(os.path.join(path, child))])
    elif path.endswith('.zip'):
        directory['children'], directory['size'] = get_zip_contents(path)
    else:
        directory['size'] = os.path.getsize(path)
    return directory

def get_zip_contents(path):
    contents = []
    with zipfile.ZipFile(path, 'r') as zip_file:
        for item in zip_file.infolist():
            item_path = item.filename.split('/')
            if len(item_path) == 1:
                contents.append({'name': item_path[0], 'path': item.filename, 'size': item.file_size})
            else:
                current_directory = contents
                for subpath in item_path[:-1]:
                    matching_directory = next((d for d in current_directory if d['name'] == subpath), None)
                    if matching_directory:
                        current_directory = matching_directory['children']
                    else:
                        new_directory = {'name': subpath, 'path': os.path.join(*item_path[:item_path.index(subpath)+1]), 'children': [], 'size': 0}
                        current_directory.append(new_directory)
                        current_directory = new_directory['children']
                current_directory.append({'name': item_path[-1], 'path': item.filename, 'size': item.file_size})
    total_size = sum([item['size'] for item in contents])
    return contents, total_size



def directory_to_json(directory_path):
    directory_structure = get_directory_structure(directory_path)
    json_data = json.dumps(directory_structure, indent=4)
    return json_data

import sys
print(directory_to_json(sys.argv[1]))
