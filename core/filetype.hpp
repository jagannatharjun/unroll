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

    static Type findType(const QString &path);
};
