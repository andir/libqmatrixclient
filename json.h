/******************************************************************************
 * Copyright (C) 2015 Kitsune Ral <kitsune-ral@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include <algorithm>
#include <functional>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonValue>
#include <QtCore/QString>
#include <QtCore/QVariant>

namespace QMatrixClient
{
    class JsonValue : public QJsonValue
    {
        public:
            JsonValue(QJsonValue v) : QJsonValue(v) { }

            template <class T>
            bool assign(T* valHolder) const
            {
                *valHolder = QVariant(*this).value<T>();
                return true;
            }
    };

//    template <>
//    bool JsonValue::assign(QString* valHolder) const
//    {
//        if (type() != QJsonValue::String)
//            return false;

//        *valHolder = toString();
//        return true;
//    }

    template <class ValT>
    class JsonNodeBase : public ValT
    {
        public:
            template <class SourceValT>
            JsonNodeBase(const SourceValT &source) : ValT(source) { }

            JsonNodeBase<ValT> operator[](QString key) const { return ValT::toObject()[key]; }
            JsonNodeBase<ValT> operator[](QJsonArray::size_type i) const { return ValT::toArray()[i]; }
    };

    using JsonNode = JsonNodeBase<QJsonValue>;
    using JsonNodeRef = JsonNodeBase<QJsonValueRef>;

    class JsonPair : public QPair<QString, JsonValue>
    {
        public:
            JsonPair(QString k, QJsonValue v) : QPair(k,v) { }
            explicit JsonPair(const QJsonObject::const_iterator& it)
                : QPair(it.key(), it.value()) { }

            QString key() const { return first; }
            JsonValue value() const { return second; }
    };

    class JsonPairRef
    {
        public:
            JsonPairRef(QJsonObject::iterator other) : iter(other) { }

            QString key() const { return iter.key(); }
            QJsonValueRef value() { return iter.value(); }
        private:
            QJsonObject::iterator iter;
    };

    class JsonObject : public QJsonObject
    {
        public:
            JsonObject() = default;
            JsonObject(const QJsonObject& o) : QJsonObject(o) { }

            template <class ValT>
            JsonObject(const JsonNodeBase<ValT>& n) : QJsonObject(n.toObject()) { }

            explicit JsonObject(const QJsonDocument& data)
                : QJsonObject (data.object()) { }

            inline JsonObject object(QString key) const
            {
                return value(key).toObject();
            }

            JsonNode operator[] (const QString &key) const { return QJsonObject::operator[](key); }
//            JsonNodeRef operator[] (const QString &key) { return QJsonObject::operator[](key); }

            using QJsonObject::contains;
            template <typename T>
            bool contains(std::initializer_list<T> keys)
            {
                return std::all_of(keys.begin(), keys.end(),
                        [=](const QString &k) { return contains(k); });
            }

            template <class T>
            bool assign(QString key, T* valHolder) const
            {
                const_iterator iter = find(key);
                if (iter == end())
                    return false;

                return iter->value().assign(valHolder);
            }

//            template <typename T, class FactoryT>
//            T* parse(QString key, FactoryT factory) const
//            {
//                const_iterator iter = find(key);
//                if (iter == end())
//                    return nullptr;

//                return factory(iter->value());
//            }

            // STL compatibility
            using value_type = JsonPair;
            using key_type = QString;
            using mapped_type = JsonValue;
        private:
            /**
             * This template class is used to change the behaviour of
             * QJsonObject iterators to be more compliant with ordinary hashmap
             * iterators. In particular, it changes the referenced item
             * (the one returned by operator*) from QJsonValue to a key-value
             * pair encapsulated into a JsonPair class. Respectively, key()
             * and value() are not provided by the iterator but rather by the
             * value_type class. This allows to use a range-based for statement
             * to iterate over key-value pairs (otherwise you have to resort
             * to the old for syntax if you need access to keys as well).
             */
            template <class IterT>
            class adjust_iterator : public IterT
            {
                protected:
                    template <class T>
                    class Holder
                    {
                        public:
                            template <typename... ParamTs>
                            Holder(ParamTs... params) : obj(params...) { }

                            inline T operator*() const { return obj; }
                            inline const T* operator->() const { return &obj; }
                        protected:
                            T obj;
                    };

                public:
                    using base_type = IterT;
//                    using typename base_type::iterator_category;
//                    using typename base_type::difference_type;
                    using value_type = JsonObject::value_type;

                    using reference = value_type&;
                    using pointer = Holder<value_type>;

                    using base_type::base_type;
                    adjust_iterator(base_type base) : base_type(base) { }

//                    operator base_type() const { return *this; }
//                    base_type baseIter() { return *this; }

                    value_type operator*() const { return value_type(*this); }
                    pointer operator->() const { return Holder<value_type>(*this); }

//                    using base_type::operator==;
//                    using base_type::operator!=;
//                    using base_type::operator++;
//                    using base_type::operator--;
//                    using base_type::operator+;
//                    using base_type::operator-;
//                    using base_type::operator+=;
//                    using base_type::operator-=;
            };

        public:
            using const_iterator = adjust_iterator<QJsonObject::const_iterator>;

            class iterator : public adjust_iterator<QJsonObject::iterator>
            {
                public:
                    using adjust_iterator::adjust_iterator;
                    using reference = JsonPairRef;
                    class pointer : public Holder<JsonPairRef>
                    {
                        public:
                            using Holder::Holder;

                            inline reference operator*() { return obj; }
                            inline reference* operator->() { return &obj; }
                    };

                    inline reference operator*() { return reference(*this); }
                    inline pointer operator->() { return pointer(*this); }
            };

        public:
            // STL style
            inline iterator begin() { return QJsonObject::begin(); }
            inline const_iterator begin() const { return QJsonObject::begin(); }
            inline const_iterator constBegin() const { return QJsonObject::constBegin(); }
            inline iterator end() { return QJsonObject::end(); }
            inline const_iterator end() const { return QJsonObject::end(); }
            inline const_iterator constEnd() const { return QJsonObject::constEnd(); }
            inline iterator erase(iterator it) { return QJsonObject::erase(it); }

            // more Qt
            typedef iterator Iterator;
            typedef const_iterator ConstIterator;
            iterator find(const QString &key) { return QJsonObject::find(key); }
            const_iterator find(const QString &key) const { return QJsonObject::find(key); }
            const_iterator constFind(const QString &key) const { return QJsonObject::constFind(key); }
            iterator insert(const QString &key, const QJsonValue &value) { return QJsonObject::insert(key, value); }
            iterator insert(const JsonPair &kv) { return insert(kv.key(), kv.value()); }

    };
}
