#pragma once


class QString;


class FileType
{
public:
    enum Type
    {
        VideoFile,
        ImageFile,
        OtherFile
    };

    Type findType(const QString &path);
};
