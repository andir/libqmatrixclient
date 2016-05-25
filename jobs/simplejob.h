#pragma once

#include "basejob.h"

#include "../json.h"

namespace QMatrixClient
{
    template <class ApiParamsT> class Job;

    class APIParams
    {
        public:
            using Endpoint = QString;
            struct Query : public QUrlQuery
            {
                using QUrlQuery::QUrlQuery;
                Query() = default;
                Query(QList<QPair<QString, QString> > l)
                {
                    setQueryItems(l);
                }
            };
            struct Data : public QJsonObject
            {
                using QJsonObject::QJsonObject;
                Data() = default;
                Data(QList<QPair<QString, QString> > l)
                {
                    for (auto i: l)
                        insert(i.first, i.second);
                }
            };

        protected:
            APIParams(QString n, JobHttpType t, Endpoint p,
                      Query q = Query(), Data d = Data())
                : m_name(n), m_type(t), m_endpoint(p), m_query(q), m_data(d) { }

        public:
            QString name() const { return m_name; }
            JobHttpType type() const { return m_type; }
            QString apiPath() const { return m_endpoint; }
            QUrlQuery query() const { return m_query; }
            QJsonObject data() const { return m_data; }
            bool needsToken() const { return true; }

            class ResultBase
            {
                public:
                    ResultBase() : m_valid(false) { }
                    template <typename T>
                    void fill(QJsonObject& json, QString key, T* valHolder)
                    {
                        auto it = json.find(key);
                        if (it == json.end())
                            qWarning() << "JSON key" << key << "not found";
                        else if (!JsonValue(it.value()).assign(valHolder))
                            qWarning() << "JSON key" << key
                                       << ", value" << it.value()
                                       << "is not loadable to"
                                       << QMetaType::typeName(qMetaTypeId<T>());
                    }

                    QString errors() const { return m_errors; }
                    bool valid() const { return m_valid; }

                protected:
                    void setValid(bool v) { m_valid = v; }

                private:
                    QString m_errors;
                    bool m_valid;
            };

        private:
            QString m_name;
            JobHttpType m_type;
            Endpoint m_endpoint;
            Query m_query;
            Data m_data;

//        private:
//            template <class> friend class Job;

//            using Result = ResultBase;
//            JobImpl<APIParams> bareParams;
    };

            template <class ApiParamsT>
            class JobImpl
            {
                public:
                    JobImpl(ApiParamsT p)
                        :params(p)
//                        Endpoint p,
//                            Query q = Query(), Data d = Data())
//                        : m_name(n), m_type(t), m_endpoint(p),
//                          m_query(q), m_data(d)
                    { }

                    JobImpl(JobImpl&) = delete;

                private:
                    ApiParamsT params;
                    QScopedPointer<typename ApiParamsT::Result> result;

                    friend class Job<ApiParamsT>;
            };

//    class ValueRef;
    
    class APIResult
    {
        public:
            using container_type = QVariantHash;
            
            QString parse(const QJsonDocument& data);

            QVariant operator[](QString key) const;

            template <typename T>
            T getAs(QString key) const
            {
                return (*this)[key].value<T>();
            }

        public:
            typedef QPair<QString, container_type&> ValueRef;

        protected:            
            void setType(QString jsonKey, QVariant::Type type);
            
            /**
             * Fills the hashmap of result items from the passed QJsonDocument.
             * The list of result items should be filled before calling this
             * function with pairs of JSON keys and empty QVariants initialized
             * with an expected type or UnknownType if any type is acceptable.
             *
             * @see ResultItem
             */
            QStringList fillResult(const QJsonDocument& data);

        private:
            // JSON key mapped to the value (already with the right type
            // inside the QVariant)
            container_type results;
    };

    class ValueRef
    {
        public:
            ValueRef(QString key);
            ValueRef operator()(QString key);
            ValueRef operator()(QVariantList::size_type index);
    };
    
    typedef QString JsonPath;
    
    /**
     * This template allows to add and access result items of jobs
     * in a type-safe way. When you derive from SimpleJob, specify
     * a member that you expect to be filled from the response JSON
     * as ResultItem<Type> field instead of "Type field;". Then in the
     * constructor of the derived class, initialize each result item
     * by calling initResultItem() with the response's JSON key you need
     * to fetch this item from.
     * Accessing the fetched item is as simple as accessing the respective
     * field as if it were a read accessor call with the same name.
     * get() method does the same, for cases when operator()()
     * cannot/may not be used.
     *
     * Only top-level JSON keys and primitive (non-object) types are
     * supported so far; support of inner keys and Event* lists is in plans.
     * Changes of result items through ResultItem<> are not allowed
     * and won't be - it's supposed to be a read-only interface.
     *
     * @see APIResult::initResultItem()
     */
//    template <typename T>
//    class ResultItem
//    {
//        public:
//            ResultItem(QString k, APIParams::container_type& cont)
//                : jsonPath(k), results(cont)
//            {
//                QVariant typeHolder { T() };
//#ifndef NDEBUG
//                auto iter = results.find(k);
//                if (iter != results.end())
//                {
//                    qWarning() << "ResultItem" << k
//                               << " already has type" << iter->typeName()
//                               << ", overwriting with" << typeHolder.typeName();
//                }
//#endif
//                results.insert(k, typeHolder);
//            }
//            explicit ResultItem(APIResult::ValueRef path)
//                : ResultItem(path.first, path.second) { }

//            const T get() const
//            {
//                return results[jsonPath.first].template value<T>();
//            }
//            const T operator()() const { return get(); }

//        private:
//            APIResult::container_type& results;
//            APIResult::ValueRef jsonPath;
//    };

    template <class ApiParamsT>
    class Job : public BaseJob
    {
        public:
            template <typename... Ts>
            static Job<ApiParamsT>* make(ConnectionData* data, Ts... params)
            {
                return new Job<ApiParamsT>(data, ApiParamsT(params...));
            }

            template <typename... Ts>
            static Job<ApiParamsT>* makeAndStart(ConnectionData* data, Ts... params)
            {
                auto job = make(data, params...);
                job->start();
                return job;
            }

            Job(ConnectionData* c, ApiParamsT params)
                : BaseJob(c, params.type(), params.name(), params.needsToken())
                , impl(params)
            { }
            Job(Job<ApiParamsT> &) = delete;
            virtual ~Job() = default;

            const typename ApiParamsT::Result * results() const { return impl.result.data(); }

        protected:
            virtual QString apiPath() const override { return impl.params.apiPath(); }

            virtual QUrlQuery query() const override { return impl.params.query(); }

            virtual QJsonObject data() const override { return impl.params.data(); }

            virtual void parseJson(const QJsonDocument& data) override
            {
                impl.result.reset(new typename ApiParamsT::Result(data));
                if (!impl.result->valid())
                {
                    setError(UserDefinedError);
                    setErrorText(impl.result->errors());
                }
                emitResult();
            }

        private:
            JobImpl<ApiParamsT> impl;
    };
}
