#pragma once
#ifndef _TEMPLATEMANAGER_HH_
#define _TEMPLATEMANAGER_HH_

#include <map>
#include <string>
#include <memory>


template<typename K, typename T>
class TemplateManager
{
 public:
	virtual ~TemplateManager()
	{
		items.clear();
	}


	virtual bool Load( const std::string& filepath ) = 0;
	virtual bool Load( const std::string& filepath, const K& key ) = 0;


	virtual void Add( const K& key, const std::shared_ptr<T> &item )
	{
		items[key] = item;
	}


	virtual std::shared_ptr<T> Get( const K& key )
	{
		return items[key];
	}


	virtual std::shared_ptr<T> Remove( const K& key )
	{
		std::shared_ptr<T> ret = items[key];
		items.erase( key );
		return ret;
	}


 protected:
	std::map< K, std::shared_ptr<T> > items;
};

#endif

